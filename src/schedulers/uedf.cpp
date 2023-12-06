#include "schedulers.h"
#include <numeric>
#include <algorithm>

void resetBudgets(std::vector<std::vector<std::pair<int,Fraction>>>& core_budgets, int cores) {
    core_budgets = std::vector<std::vector<std::pair<int,Fraction>>>(cores);
};

void UEDF::init(const TaskSet& task_set, int cores) {
    next_event = 0;
    resetBudgets(core_budgets, cores);
    task_next_job = std::vector<int>(task_set.size(), -1);
}

ScheduleDecision UEDF::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    bool new_job = false;
    for (int i = 0; i < model.task_set.size(); ++i) {
        if (model.task_set[i].next_job_id != task_next_job[i]) {
            task_next_job[i] = model.task_set[i].next_job_id;
            new_job = true;
        }
    }

    // allocates task budgets to cores
    if (new_job) {
        resetBudgets(core_budgets, model.cores);
        next_event = nextJobRelease(model.task_set, model.active_jobs, model.time);
        std::vector<int> ordered_tasks(model.task_set.size());
        std::vector<Fraction> task_deadline(model.task_set.size(), INT_MAX);
        for (const Job& job : model.active_jobs) {
            task_deadline[job.task_id] = job.deadline;
        }
        std::iota(ordered_tasks.begin(), ordered_tasks.end(), 0);
        auto cmp = [&](int i, int j) {
            return task_deadline[i] < task_deadline[j];
        };
        std::sort(ordered_tasks.begin(), ordered_tasks.end(), cmp);
        Fraction delta_time = next_event - model.time;
        std::vector<Fraction> core_budget(model.cores, delta_time);
        int core = 0;
        for (int tid : ordered_tasks) {
            const Task& task = model.task_set[tid];
            Fraction task_budget = delta_time * (task.exec_time / task.period);
            while (task_budget > 0) {
                if (core_budget[core] == 0 && ++core >= model.cores) break;
                Fraction alloc_amount = std::min(task_budget, core_budget[core]);
                core_budgets[core].emplace_back(tid, alloc_amount);
                task_budget -= alloc_amount;
                core_budget[core] -= alloc_amount;
            }
        }
    }

    // schedule
    std::vector<int> task_core(model.task_set.size(), -2); // -2 if not active, -1 if not scheduled
    std::vector<int> core_budget_index(model.cores, -1);
    for (const Job& job : model.active_jobs)
        task_core[job.task_id] = -1;
    for (int core = 0; core < model.cores; ++core) {
        for (int budget_index = 0; budget_index < core_budgets[core].size(); ++budget_index) {
            auto& budget = core_budgets[core][budget_index];
            if (task_core[budget.first] != -1 || budget.second == 0) continue;
            task_core[budget.first] = core;
            core_budget_index[core] = budget_index;
            break;
        }
    }
    std::vector<int> chosen_jobs;
    for (int i = 0; i < model.active_jobs.size(); ++i) {
        const Job& job = model.active_jobs[i];
        if (task_core[job.task_id] != -1)
            chosen_jobs.push_back(i);
    }
    assignToCores(model.active_jobs, sd.core_state, chosen_jobs);
    sd.next_event = nextSchedEvent(model.task_set, model.active_jobs, model.time);
    for (int core = 0; core < model.cores; ++core) {
        int budget_index = core_budget_index[core];
        if (budget_index == -1) continue;
        sd.next_event = std::min(sd.next_event, model.time + core_budgets[core][budget_index].second);
    }

    // update budgets
    Fraction delta_time = sd.next_event - model.time;
    for (int core = 0; core < model.cores; ++core) {
        int budget_index = core_budget_index[core];
        if (budget_index == -1) continue;
        core_budgets[core][budget_index].second -= delta_time;
    }

    return sd;
}