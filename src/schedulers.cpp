#include "schedulers.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <iostream>

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
template<class T, class D>
std::vector<int> choose_by_priority(const JobSet& active_jobs, int cores, int time, T priority_threshold, D associated_data, T (*priority)(const Job&, int, D)) {
    std::vector<int> chosen_jobs;
    std::vector<T> job_priorities;
    job_priorities.reserve(active_jobs.size());
    for (const Job& job : active_jobs)
        job_priorities.push_back(priority(job, time, associated_data));
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

CoreState TEMP::schedule(const JobSet& active_jobs, int cores, int time) {
    CoreState core_state(cores, -1);
    int cap = std::min(cores, (int)active_jobs.size());
    for (int i = 0; i < cap; ++i)
        core_state[i] = i;
    return core_state;
}

CoreState GRM::schedule(const JobSet& active_jobs, int cores, int time) {
    CoreState core_state(cores, -1);
    assign_to_cores(active_jobs, core_state, choose_by_priority<int,bool>(active_jobs, cores, time, INT_MIN, false, [](const Job& job, int time, bool) {
        return -job.period;
    }));
    return core_state;
}

CoreState GEDF::schedule(const JobSet& active_jobs, int cores, int time) {
    CoreState core_state(cores, -1);
    assign_to_cores(active_jobs, core_state, choose_by_priority<int,bool>(active_jobs, cores, time, INT_MIN, false, [](const Job& job, int time, bool) {
        return -job.deadline;
    }));
    return core_state;
}

CoreState GFIFO::schedule(const JobSet& active_jobs, int cores, int time) {
    CoreState core_state(cores, -1);
    assign_to_cores(active_jobs, core_state, choose_by_priority<int,bool>(active_jobs, cores, time, INT_MIN, false, [](const Job& job, int time, bool) {
        return 0;
    }));
    return core_state;
}

CoreState PD2::schedule(const JobSet& active_jobs, int cores, int time) {
    // calculate priority of each job
    // pick k highest priorities
    CoreState core_state(cores, -1);
    assign_to_cores(active_jobs, core_state, choose_by_priority<long long,bool>(active_jobs, cores, time, 0, early_release, [](const Job& job, int time, bool early_release) {
        auto get_itv = [&job](int work_done) {
            int rel_deadline = job.deadline - job.release_time;
            return std::make_pair(
                job.release_time + std::max(0, ((work_done - 1) * rel_deadline + job.exec_time) / job.exec_time - 1),
                job.release_time + std::min(rel_deadline - 1, (work_done * rel_deadline + job.exec_time - 1) / job.exec_time - 1)
            );
        };
        std::pair<int,int> first_itv = get_itv(job.runtime + 1);

        // handle early releasing
        if (early_release)
            first_itv.first = 0;
        else if (first_itv.first > time)
            return (long long)-1;

        // generate intervals to next group deadline
        int curr_work = job.runtime;
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
        long long priority = (long long)(INT_MAX - first_itv.second) << 32; // deadline of current interval
        if (job.exec_time + job.exec_time >= job.deadline) // heavy task
            priority += (long long)1 << 31;
        if (overlapping_next) // first itv overlapping next
            priority += (long long)1 << 30;

        while (curr_work < job.exec_time && overlapping_next && curr_itv_len == 2)
            step_itv();
        priority += curr_itv.first + 1; // next group deadline
        return priority;
    }));
    return core_state;
}