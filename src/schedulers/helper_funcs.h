#ifndef SCHED_HELPER_FUNCS_H
#define SCHED_HELPER_FUNCS_H

#include "schedulers.h"

#include <cassert>
#include <algorithm>
#include <functional>

// helper function to assign chosen jobs to cores
void assignToCores(const JobSet& active_jobs, CoreState& core_state, std::vector<int> chosen_jobs);

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
Fraction nextJobRelease(const TaskSet& task_set, const JobSet& active_jobs, Fraction time);
Fraction nextJobDeadline(const TaskSet& task_set, const JobSet& active_jobs, Fraction time);
Fraction nextSchedEvent(const TaskSet& task_set, const JobSet& active_jobs, Fraction time);
Fraction nextJobCompletion(const JobSet& active_jobs, const CoreState& core_state, Fraction time);

bool usesIntegerTime(const TaskSet& task_set);


#endif