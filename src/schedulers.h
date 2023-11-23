#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <vector>
#include "model.h"

// global EDF
struct GEDF : public Scheduler {
    GEDF(bool preemptive) : Scheduler(PriorityScheme::JOB_LEVEL_DYN, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED, DecisionType::EVENT_BASED) {}
    CoreState schedule(const JobSet& active_jobs, int cores, int time) override;
};

// global RM
struct GRM : public Scheduler {
    GRM(bool preemptive) : Scheduler(PriorityScheme::STATIC, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED, DecisionType::EVENT_BASED) {}
    CoreState schedule(const JobSet& active_jobs, int cores, int time) override;
};

// global FIFO
struct GFIFO : public Scheduler {
    GFIFO() : Scheduler(PriorityScheme::STATIC, MigrationDegree::RESTRICTED, DecisionType::EVENT_BASED) {}
    CoreState schedule(const JobSet& active_jobs, int cores, int time) override;
};

// testing sheduler (schedules by job id)
struct TEMP : public Scheduler {
    TEMP(bool preemptive) : Scheduler(PriorityScheme::STATIC, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED, DecisionType::EVENT_BASED) {}
    CoreState schedule(const JobSet& active_jobs, int cores, int time) override;
};

// partitioned preemptive EDF
struct PEDF : public Scheduler {
    PEDF(int cores) : Scheduler(PriorityScheme::JOB_LEVEL_DYN, MigrationDegree::PARTITIONED, DecisionType::EVENT_BASED) {}
};

// PD2 with optional Early Releasing and Intra Sporadic
struct PD2 : public Scheduler {
    bool early_release;
    PD2(bool early_release = true) : Scheduler(PriorityScheme::UNRESTRICTED_DYN, MigrationDegree::FULL, DecisionType::QUANTUM_BASED), early_release(early_release) {}
    CoreState schedule(const JobSet& active_jobs, int cores, int time) override;
};

#endif