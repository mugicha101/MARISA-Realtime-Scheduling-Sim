#include "schedulers.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <climits>
#include <functional>

// helper function to assign chosen jobs to cores
void assign_to_cores(const JobSet& active_jobs, CoreState& core_state, std::vector<int> chosen_jobs) {
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
template<class T>
std::vector<int> choose_by_priority(const JobSet& active_jobs, int cores, T priority_threshold, std::function<T(const Job& job)> priority_func) {
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
        if (job_priorities[i] < priority_threshold)
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

CoreState TEMP::schedule(const JobSet& active_jobs, int cores, Fraction time) {
    CoreState core_state(cores, -1);
    int cap = std::min(cores, (int)active_jobs.size());
    for (int i = 0; i < cap; ++i)
        core_state[i] = i;
    return core_state;
}

CoreState GRM::schedule(const JobSet& active_jobs, int cores, Fraction time) {
    CoreState core_state(cores, -1);
    auto priority_func = [](const Job& job) {
        return -job.period;
    };
    assign_to_cores(active_jobs, core_state, choose_by_priority<Fraction>(active_jobs, cores, INT_MIN, priority_func));
    return core_state;
}

CoreState GEDF::schedule(const JobSet& active_jobs, int cores, Fraction time) {
    CoreState core_state(cores, -1);
    auto priority_func = [](const Job& job) {
        return -job.deadline;
    };
    assign_to_cores(active_jobs, core_state, choose_by_priority<Fraction>(active_jobs, cores, INT_MIN, priority_func));
    return core_state;
}

CoreState GFIFO::schedule(const JobSet& active_jobs, int cores, Fraction time) {
    CoreState core_state(cores, -1);
    auto priority_func = [](const Job& job) {
        return 0;
    };
    assign_to_cores(active_jobs, core_state, choose_by_priority<int>(active_jobs, cores, INT_MIN, priority_func));
    return core_state;
}

CoreState PD2::schedule(const JobSet& active_jobs, int cores, Fraction time) {
    CoreState core_state(cores, -1);
    bool early_release = this->early_release;
    auto priority_func = [early_release, time](const Job& job) {
        if (!job.exec_time.isInt() || !job.deadline.isInt() || !job.period.isInt() || !job.release_time.isInt() || !job.runtime.isInt())
            throw("PD2 recieved non-integer times");
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
    assign_to_cores(active_jobs, core_state, choose_by_priority<long long>(active_jobs, cores, 0, priority_func));
    return core_state;
}