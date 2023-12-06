#include "schedulers.h"

void GLLF::init(const TaskSet& task_set, int cores) {
    valid_task_set = usesIntegerTime(task_set);
}

ScheduleDecision GLLF::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    if (!valid_task_set) return sd; // don't schedule if tasks don't use integer time
    auto priority_func = [](const Job& job) {
        return -(job.deadline - (job.exec_time - job.runtime));
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<Fraction>(model.active_jobs, model.cores, INT_MIN, priority_func));
    sd.next_event = model.time + 1;
    return sd;
}
