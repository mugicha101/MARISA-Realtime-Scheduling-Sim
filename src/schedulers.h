#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include <vector>
#include "model.h"

// global EDF
struct GEDF : public Scheduler {
    GEDF(bool preemptive) : Scheduler(PriorityScheme::JOB_LEVEL_DYN, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED, ScheduleEvent::JOB_RELEASE) {}
    CoreState schedule(const JobSet& active_jobs, int cores) override;
};

// global RM
struct GRM : public Scheduler {
    GRM(bool preemptive) : Scheduler(PriorityScheme::STATIC, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED, ScheduleEvent::JOB_RELEASE) {}
    CoreState schedule(const JobSet& active_jobs, int cores) override;
};

// testing sheduler (schedules by job id)
struct TEMP : public Scheduler {
    TEMP(bool preemptive) : Scheduler(PriorityScheme::STATIC, preemptive ? MigrationDegree::FULL : MigrationDegree::RESTRICTED, ScheduleEvent::JOB_RELEASE) {}
    CoreState schedule(const JobSet& active_jobs, int cores) override;
};

// partitioned preemptive EDF
struct PEDF : public Scheduler {
    PEDF(int cores) : Scheduler(PriorityScheme::JOB_LEVEL_DYN, MigrationDegree::PARTITIONED, ScheduleEvent::JOB_RELEASE) {}
};

// PD2 with Intra Sporadic
struct PD2 : public Scheduler {
    PD2(int cores) : Scheduler(PriorityScheme::UNRESTRICTED_DYN, MigrationDegree::FULL, ScheduleEvent::QUANTUM) {}
};

#endif