#include "helper_funcs.h"

#include <cassert>
#include <algorithm>
#include <functional>

// helper function to assign chosen jobs to cores
void assignToCores(const JobSet& active_jobs, CoreState& core_state, std::vector<int> chosen_jobs) {
    assert(chosen_jobs.size() <= core_state.size());
    sort(chosen_jobs.begin(), chosen_jobs.end());
    
    // reassign chosen jobs already executing (mitigates context switches)
    for (int i : chosen_jobs)
        if (active_jobs[i].running)
            core_state[active_jobs[i].core] = i;

    // assign chosen jobs not executing
    int next_empty = -1;
    for (int i : chosen_jobs) {
        // skip executing jobs
        if (active_jobs[i].running)
            continue;
        
        // prioritize previous core ran on (mitigates job-level migrations)
        // if taken by job that is either fresh or migrating, swap places
        if (active_jobs[i].core != -1) {
            int prev_core = active_jobs[i].core;
            if (core_state[prev_core] == -1) {
                // assign to prev core
                core_state[prev_core] = i;
                continue;
            } else if (active_jobs[core_state[prev_core]].core != prev_core) {
                // swap with job on core
                std::swap(core_state[prev_core], i);
            }
        }

        // assign to empty core
        while (core_state[++next_empty] != -1);
        core_state[next_empty] = i;
    }
};

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

bool usesIntegerTime(const TaskSet& task_set) {
    for (const Task& task : task_set)
        if (!task.phase.isInt() || !task.period.isInt() || !task.exec_time.isInt() || !task.relative_deadline.isInt())
            return false;
    return true;
};
