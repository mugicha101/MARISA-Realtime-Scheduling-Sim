#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <vector>
#include <unordered_map>
#include "../model.h"
#include "helper_funcs.h"

// Global Eearliest Deadline First
struct GEDF : public Scheduler {
    GEDF() : Scheduler(PriorityScheme::JOB_LEVEL_DYN, MigrationDegree::FULL) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// Global LLF on Discrete Time
struct GLLF : public Scheduler {
    bool valid_task_set = false;
    GLLF() : Scheduler(PriorityScheme::JOB_LEVEL_DYN, MigrationDegree::FULL) {}
    ScheduleDecision schedule(const SimModel& model) override;
    void init(const TaskSet& task_set) override;
};

// Global Deadline Monotonic (Rate Monotonic if implicit deadlines used)
struct GDM : public Scheduler {
    GDM() : Scheduler(PriorityScheme::STATIC, MigrationDegree::FULL) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// Global First In First Out
struct GFIFO : public Scheduler {
    GFIFO() : Scheduler(PriorityScheme::STATIC, MigrationDegree::RESTRICTED) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// Earliest Deadline First until Zero Laxity
struct EDZL : public Scheduler {
    EDZL() : Scheduler(PriorityScheme::JOB_LEVEL_DYN, MigrationDegree::FULL) {}
    ScheduleDecision schedule(const SimModel& model) override;
};

// PD2 with Intra Sporadic and optional Early Releasing on Discrete Time
struct PD2 : public Scheduler {
    bool early_release;
    bool valid_task_set = false;
    PD2(bool early_release = true) : Scheduler(PriorityScheme::UNRESTRICTED_DYN, MigrationDegree::FULL), early_release(early_release) {}
    ScheduleDecision schedule(const SimModel& model) override;
    void init(const TaskSet& task_set) override;
};

// Largest Lowest Remaining Execution First
struct LLREF : public Scheduler {
    Fraction next_event;
    std::unordered_map<long long, Fraction> local_exec;
    LLREF() : Scheduler(PriorityScheme::UNRESTRICTED_DYN, MigrationDegree::FULL) {}
    ScheduleDecision schedule(const SimModel& model) override;
    void init(const TaskSet& task_set) override;
};

#endif