#include "schedulers.h"

ScheduleDecision EDZL::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    auto priority_func = [&time = model.time](const Job& job) {
        return job.deadline - time == job.exec_time - job.runtime ? Fraction(INT_MAX) : -job.deadline;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<Fraction>(model.active_jobs, model.cores, INT_MIN, priority_func));
    sd.next_event = std::min(nextSchedEvent(model.task_set, model.active_jobs, model.time), nextJobCompletion(model.active_jobs, sd.core_state, model.time));
    std::vector<bool> scheduled(model.active_jobs.size(), false);
    for (int i : sd.core_state) {
        if (i == -1) continue;
        scheduled[i] = true;
    }
    for (int i = 0; i < model.active_jobs.size(); ++i) {
        if (scheduled[i]) continue;
        const Job& job = model.active_jobs[i];
        Fraction event = job.deadline - (job.exec_time - job.runtime);
        if (event > model.time) sd.next_event = std::min(sd.next_event, event);
    }
    return sd;
}
