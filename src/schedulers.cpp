#include "schedulers.h"

#include <vector>
#include <numeric>
#include <algorithm>

CoreState TEMP::schedule(const JobSet& active_jobs, int cores) {
    CoreState core_state(cores, -1);
    int cap = std::min(cores, (int)active_jobs.size());
    for (int i = 0; i < cap; ++i)
        core_state[i] = i;
    return core_state;
}

CoreState GRM::schedule(const JobSet& active_jobs, int cores) {
    CoreState core_state(cores, -1);
    std::vector<int> chosen_jobs;
    auto cmp = [&](int i, int j) {
        return active_jobs[i].period == active_jobs[j].period ? i < j : active_jobs[i].period < active_jobs[j].period;
    };
    auto query_heap = [&](int job_index) {
        chosen_jobs.push_back(job_index);
        std::push_heap(chosen_jobs.begin(), chosen_jobs.end(), cmp);
        if (chosen_jobs.size() == cores + 1) {
            std::pop_heap(chosen_jobs.begin(), chosen_jobs.end(), cmp);
            chosen_jobs.pop_back();
        }
    };

    // consider executing jobs
    for (int i = 0; i < active_jobs.size(); ++i)
        if (active_jobs[i].running)
            query_heap(i);

    // consider preempted jobs
    for (int i = 0; i < active_jobs.size(); ++i)
        if (!active_jobs[i].running && active_jobs[i].core != -1)
            query_heap(i);

    // consider unexecuted jobs
    for (int i = 0; i < active_jobs.size(); ++i)
        if (active_jobs[i].core == -1)
            query_heap(i);
            
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

    return core_state;
}