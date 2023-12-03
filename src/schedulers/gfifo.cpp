#include "schedulers.h"

ScheduleDecision GFIFO::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    auto priority_func = [](const Job& job) {
        return 0;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<int>(model.active_jobs, model.cores, INT_MIN, priority_func));
    sd.next_event = std::min(nextSchedEvent(model.task_set, model.active_jobs, model.time), nextJobCompletion(model.active_jobs, sd.core_state, model.time));
    return sd;
}