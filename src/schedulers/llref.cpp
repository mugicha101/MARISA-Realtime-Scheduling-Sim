#include "schedulers.h"

void LLREF::init(const TaskSet& task_set) {
    next_event = 0;
    local_exec.clear();
}

ScheduleDecision LLREF::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    sd.next_event = nextSchedEvent(model.task_set, model.active_jobs, model.time);

    // enter next TL plane
    if (sd.next_event > next_event) {
        Fraction tl_time = sd.next_event - next_event;
        local_exec.clear();
        for (const Job& job : model.active_jobs)
            local_exec[job.uid] = tl_time * (job.exec_time / (job.deadline - job.release_time));
        next_event = sd.next_event;
    }

    // schedule by max remaining local exec time
    auto priority_func = [&local_exec = local_exec](const Job& job) {
        return local_exec[job.uid];
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<Fraction>(model.active_jobs, model.cores, 0, priority_func));

    // find next secondary event
    std::vector<Fraction> next_secondary(model.active_jobs.size(), -1);
    for (int i : sd.core_state) {
        if (i == -1) continue;
        next_secondary[i] = model.time + local_exec[model.active_jobs[i].uid];
    }
    for (int i = 0; i < model.active_jobs.size(); ++i) {
        if (next_secondary[i] != -1) continue;
        next_secondary[i] = next_event - local_exec[model.active_jobs[i].uid];
    }
    for (int i = 0; i < model.active_jobs.size(); ++i) {
        if (next_secondary[i] <= model.time || next_secondary[i] >= sd.next_event) continue;
        sd.next_event = next_secondary[i];
    }

    // update local exec times
    for (int i : sd.core_state)
        local_exec[model.active_jobs[i].uid] -= sd.next_event - model.time;
    return sd;
}