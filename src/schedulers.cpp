#include "schedulers.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <climits>
#include <functional>

// helper function to assign chosen jobs to cores
void assignToCores(const JobSet& active_jobs, CoreState& core_state, std::vector<int> chosen_jobs) {
    assert(chosen_jobs.size() <= core_state.size());
    sort(chosen_jobs.begin(), chosen_jobs.end());
    
    // reassign chosen jobs already executing
    for (int i : chosen_jobs)
        if (active_jobs[i].running)
            core_state[active_jobs[i].core] = i;

    // assign chosen jobs not executing
    int next_empty = -1;
    for (int i : chosen_jobs) {
        // skip executing jobs
        if (active_jobs[i].running)
            continue;
        
        // prioritize previous core ran on (reduces migrations)
        if (active_jobs[i].core != -1 && core_state[active_jobs[i].core] == -1) {
            core_state[active_jobs[i].core] = i;
            continue;
        }

        // assign to empty core
        while (core_state[++next_empty] != -1);
        core_state[next_empty] = i;
    }
};

// helper function to choose by highest priority then lowest index
// priority must be greater than priority_threshold to schedule
template<class T>
std::vector<int> chooseByPriority(const JobSet& active_jobs, int cores, T priority_threshold, std::function<T(const Job& job)> priority_func) {
    std::vector<int> chosen_jobs;
    std::vector<T> job_priorities;
    job_priorities.reserve(active_jobs.size());
    for (const Job& job : active_jobs) {
        job_priorities.push_back(priority_func(job));
    }
    auto cmp = [&job_priorities](int i, int j) {
        return job_priorities[i] == job_priorities[j] ? i < j : job_priorities[i] > job_priorities[j];
    };
    for (int i = 0; i < active_jobs.size(); ++i) {
        if (job_priorities[i] <= priority_threshold)
            continue;
        chosen_jobs.push_back(i);
        std::push_heap(chosen_jobs.begin(), chosen_jobs.end(), cmp);
        if (chosen_jobs.size() == cores + 1) {
            std::pop_heap(chosen_jobs.begin(), chosen_jobs.end(), cmp);
            chosen_jobs.pop_back();
        }
    }
    return chosen_jobs;
}

// helper function for getting the next scheduling event
Fraction nextSchedEvent(const TaskSet& task_set, const JobSet& active_jobs, Fraction time) {
    Fraction next_event = INT_MAX;
    // job release
    for (const Task& task : task_set)
        next_event = std::min(next_event, task.next_release);
    // job deadline
    for (const Job& job : active_jobs)
        next_event = std::min(next_event, job.deadline);
    return next_event;
}

Fraction nextJobCompletion(const JobSet& active_jobs, const CoreState& core_state, Fraction time) {
    Fraction next_completion = INT_MAX;
    for (int i : core_state) {
        if (i == -1) continue;
        const Job& job = active_jobs[i];
        next_completion = std::min(next_completion, time + job.exec_time - job.runtime);
    }
    return next_completion;
}

ScheduleDecision GRM::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    auto priority_func = [](const Job& job) {
        return -job.period;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<Fraction>(model.active_jobs, model.cores, INT_MIN, priority_func));
    sd.next_event = std::min(nextSchedEvent(model.task_set, model.active_jobs, model.time), nextJobCompletion(model.active_jobs, sd.core_state, model.time));
    return sd;
}

ScheduleDecision GEDF::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    auto priority_func = [](const Job& job) {
        return -job.deadline;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<Fraction>(model.active_jobs, model.cores, INT_MIN, priority_func));
    sd.next_event = std::min(nextSchedEvent(model.task_set, model.active_jobs, model.time), nextJobCompletion(model.active_jobs, sd.core_state, model.time));
    return sd;
}

ScheduleDecision GFIFO::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    auto priority_func = [](const Job& job) {
        return 0;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<int>(model.active_jobs, model.cores, INT_MIN, priority_func));
    sd.next_event = std::min(nextSchedEvent(model.task_set, model.active_jobs, model.time), nextJobCompletion(model.active_jobs, sd.core_state, model.time));
    return sd;
}

ScheduleDecision PD2::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    // don't schedule if jobs don't use integer time
    for (const Job& job : model.active_jobs) {
        if (!job.exec_time.isInt() || !job.deadline.isInt() || !job.period.isInt() || !job.release_time.isInt() || !job.runtime.isInt())
            return sd;
    }
    auto priority_func = [early_release = this->early_release, time = model.time](const Job& job) {
        auto get_itv = [&job](int work_done) {
            int rel_deadline = job.deadline.getNum() - job.release_time.getNum();
            return std::make_pair(
                (int)job.release_time.getNum() + std::max(0, ((work_done - 1) * rel_deadline + (int)job.exec_time.getNum()) / (int)job.exec_time.getNum() - 1),
                (int)job.release_time.getNum() + std::min(rel_deadline - 1, (work_done * rel_deadline + (int)job.exec_time.getNum() - 1) / (int)job.exec_time.getNum() - 1)
            );
        };
        std::pair<int,int> first_itv = get_itv((int)job.runtime.getNum() + 1);

        // handle early releasing
        if (early_release)
            first_itv.first = 0;
        else if (first_itv.first > time)
            return (long long)-1;

        // generate intervals to next group deadline
        int curr_work = job.runtime.getNum();
        std::pair<int,int> curr_itv, next_itv = first_itv;
        bool overlapping_next;
        int curr_itv_len;
        auto step_itv = [&]() {
            curr_itv = next_itv;
            next_itv = get_itv(++curr_work + 1);
            overlapping_next = curr_itv.second == next_itv.first;
            curr_itv_len = curr_itv.second + 1 - curr_itv.first;
        };
        step_itv();

        // priority order: deadline, is heavy, itv overlaps next, next group deadline
        // priority bitstring (64 bits):
        //   32b - deadline
        //    1b - is heavy task
        //    1b - first interval overlaps next
        //   30b - next group deadline
        long long priority = (long long)(INT_MAX - first_itv.second) << 32; // deadline of current interval
        if (job.exec_time + job.exec_time >= job.deadline) // heavy task
            priority += (long long)1 << 31;
        if (overlapping_next) // first itv overlapping next
            priority += (long long)1 << 30;

        while (curr_work < job.exec_time && overlapping_next && curr_itv_len == 2)
            step_itv();
        priority += curr_itv.first + 1; // next group deadline
        return priority;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<long long>(model.active_jobs, model.cores, -1, priority_func));
    sd.next_event = model.time + 1;
    return sd;
}

void LLREF::init(const TaskSet& task_set) {
    next_event = 0;
    local_exec.clear();
}

ScheduleDecision LLREF::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    sd.next_event = nextSchedEvent(model.task_set, model.active_jobs, model.time);

    // enter next TL plane
    if (sd.next_event > next_event) {
        Fraction tl_time = sd.next_event - next_event;
        local_exec.clear();
        for (const Job& job : model.active_jobs)
            local_exec[job.uid] = tl_time * (job.exec_time / (job.deadline - job.release_time));
        next_event = sd.next_event;
    }

    // schedule by max remaining local exec time
    auto priority_func = [&local_exec = local_exec](const Job& job) {
        return local_exec[job.uid];
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<Fraction>(model.active_jobs, model.cores, 0, priority_func));

    // find next secondary event
    std::vector<Fraction> next_secondary(model.active_jobs.size(), -1);
    for (int i : sd.core_state) {
        if (i == -1) continue;
        next_secondary[i] = model.time + local_exec[model.active_jobs[i].uid];
    }
    for (int i = 0; i < model.active_jobs.size(); ++i) {
        if (next_secondary[i] != -1) continue;
        next_secondary[i] = next_event - local_exec[model.active_jobs[i].uid];
    }
    for (int i = 0; i < model.active_jobs.size(); ++i) {
        if (next_secondary[i] <= model.time || next_secondary[i] >= sd.next_event) continue;
        sd.next_event = next_secondary[i];
    }

    // update local exec times
    for (int i : sd.core_state)
        local_exec[model.active_jobs[i].uid] -= sd.next_event - model.time;
    return sd;
}