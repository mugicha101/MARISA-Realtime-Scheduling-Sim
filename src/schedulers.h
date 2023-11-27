#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <vector>
#include <unordered_map>
#include "model.h"

// global EDF
struct GEDF : public Scheduler {
    GEDF(bool preemptive) : Scheduler(PriorityScheme::JOB_LEVEL_DYN, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// global RM
struct GRM : public Scheduler {
    GRM(bool preemptive) : Scheduler(PriorityScheme::STATIC, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// global FIFO
struct GFIFO : public Scheduler {
    GFIFO() : Scheduler(PriorityScheme::STATIC, MigrationDegree::RESTRICTED) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// partitioned preemptive EDF
struct PEDF : public Scheduler {
    PEDF(int cores) : Scheduler(PriorityScheme::JOB_LEVEL_DYN, MigrationDegree::PARTITIONED) {}
};

// PD2 with Intra Sporadic and optional Early Releasing
struct PD2 : public Scheduler {
    bool early_release;
    PD2(bool early_release = true) : Scheduler(PriorityScheme::UNRESTRICTED_DYN, MigrationDegree::FULL), early_release(early_release) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// LLREF
struct LLREF : public Scheduler {
    Fraction next_event;
    std::unordered_map<long long, Fraction> local_exec;
    LLREF() : Scheduler(PriorityScheme::UNRESTRICTED_DYN, MigrationDegree::FULL) {}
    ScheduleDecision schedule(const SimModel& model) override;
    void init(const TaskSet& task_set) override;
};

#endif