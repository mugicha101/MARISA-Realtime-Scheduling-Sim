#ifndef MODEL_H
#define MODEL_H

#include "fraction.h"
#include <vector>
#include <memory>
#include <deque>
#include <climits>

struct SimModel;

struct Job {
    long long uid;
    int task_id;
    int job_id;
    Fraction period, release_time, exec_time, deadline; // basic job/task stats
    Fraction runtime = 0; // time job has executed for
    int core = -1; // core the job was last on (or currently on if running) (-1 if not executed yet)
    bool running = false; // true if the job is currently running
    Job(int task_id = -1, int job_id = -1, Fraction period = 0, Fraction release_time = 0, Fraction exec_time = 0, Fraction deadline = 0) : uid((((long long)task_id) << 32) | job_id), task_id(task_id), job_id(job_id), period(period), release_time(release_time), exec_time(exec_time), deadline(deadline) {}
};

struct Task {
    Fraction phase, period, exec_time, relative_deadline;
    int next_job_id = 0;
    Fraction next_release;
    Task(Fraction phase, Fraction period, Fraction exec_time, Fraction relative_deadline) : phase(phase), period(period), exec_time(exec_time), relative_deadline(relative_deadline), next_release(phase) {}
    Task() : Task(0, 0, 0, 0) {}
    Task(Fraction period, Fraction exec_time, Fraction relative_deadline) : Task(0, period, exec_time, relative_deadline) {}
    Task(Fraction period, Fraction exec_time) : Task(0, period, exec_time, period) {}

    // create next job for this task (task_id = id of this task which is handled externally)
    Job next_job(int task_id);
};

typedef std::vector<Task> TaskSet; // task_set[i] = task with task id i
typedef std::vector<Job> JobSet; // job_set[i] = job with job id i
typedef std::vector<int> CoreState; // core_state[i] = job id of job scheduled on core i (-1 if idle)

struct ScheduleDecision {
    Fraction next_event = INT_MAX;
    CoreState core_state;
    ScheduleDecision(int cores) : core_state(CoreState(cores, -1)) {}
};

struct ExecBlock {
    enum EndState {PREEMPTED, COMPLETED, MISSED};
    int task_id, job_id; // task and job that executed
    int core; // core executed on
    Fraction start; // start time
    Fraction end; // end time
    EndState endState; // state of end
    ExecBlock(int task_id, int job_id, int core, Fraction start, Fraction end, EndState endState) : task_id(task_id), job_id(job_id), core(core), start(start), end(end), endState(endState) {}
};

// holds exec blocks and handles adding new ones
class ExecBlockStorage {
    std::deque<ExecBlock> exec_blocks;
    std::deque<ExecBlock> new_blocks;
public:
    ExecBlockStorage() {}

    // add a block to storage
    void add_block(const Job& job, Fraction start, Fraction end);

    // empty out storage
    void clear();

    // has new block
    bool hasNext();

    // get next new block, move it to exec_blocks
    ExecBlock getNext();
};

struct Scheduler {
    enum PriorityScheme { STATIC, JOB_LEVEL_DYN, UNRESTRICTED_DYN };
    enum MigrationDegree { PARTITIONED, RESTRICTED, FULL};

    const PriorityScheme priority_scheme;
    const MigrationDegree migration_degree;
    Scheduler(PriorityScheme priority_scheme, MigrationDegree migration_degree) : priority_scheme(priority_scheme), migration_degree(migration_degree) {}

    virtual void init(const TaskSet& task_set);

    // assign jobs to cores
    virtual ScheduleDecision schedule(const SimModel& model);
};

struct SimModel {
    TaskSet task_set;
    Scheduler* scheduler = nullptr;
    ExecBlockStorage ebs;

    Fraction time = 0; // time of next unhandled scheduling decision
    int missed = -1; // missed job time (-1 if none)
    int cores = 1; // number of CPU cores available

    JobSet active_jobs;
    JobSet finished_jobs;

    SimModel() {}
    
    // reset and init task sim with given task set and scheduler
    void reset(TaskSet task_set, Scheduler* scheduler, int cores);

    // simulates to at least endTime (ignore if endTime <= buffer)
    // handles execBlocks, finding next event, and updating job object bookkeeping 
    void sim(Fraction endTime);
};

#endif