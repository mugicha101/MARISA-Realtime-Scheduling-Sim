#include "schedulers.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>

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
std::vector<int> choose_by_priority(const JobSet& active_jobs, int cores, int (*priority)(const Job&)) {
    std::vector<int> chosen_jobs;
    auto cmp = [&active_jobs, &priority](int i, int j) {
        return priority(active_jobs[i]) == priority(active_jobs[j]) ? i < j : priority(active_jobs[i]) > priority(active_jobs[j]);
    };
    for (int i = 0; i < active_jobs.size(); ++i) {
        chosen_jobs.push_back(i);
        std::push_heap(chosen_jobs.begin(), chosen_jobs.end(), cmp);
        if (chosen_jobs.size() == cores + 1) {
            std::pop_heap(chosen_jobs.begin(), chosen_jobs.end(), cmp);
            chosen_jobs.pop_back();
        }
    }
    return chosen_jobs;
}

CoreState TEMP::schedule(const JobSet& active_jobs, int cores) {
    CoreState core_state(cores, -1);
    int cap = std::min(cores, (int)active_jobs.size());
    for (int i = 0; i < cap; ++i)
        core_state[i] = i;
    return core_state;
}

CoreState GRM::schedule(const JobSet& active_jobs, int cores) {
    CoreState core_state(cores, -1);
    assign_to_cores(active_jobs, core_state, choose_by_priority(active_jobs, cores, [](const Job& job) {
        return -job.period;
    }));
    return core_state;
}

CoreState GEDF::schedule(const JobSet& active_jobs, int cores) {
    CoreState core_state(cores, -1);
    assign_to_cores(active_jobs, core_state, choose_by_priority(active_jobs, cores, [](const Job& job) {
        return -job.deadline;
    }));
    return core_state;
}