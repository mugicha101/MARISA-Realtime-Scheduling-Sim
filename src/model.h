#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <memory>
#include <deque>

struct Job {
    int task_id;
    int job_id;
    int period, release_time, exec_time, deadline; // basic job/task stats
    int runtime = 0; // time job has executed for
    int finishTime = -1; // time job finishes (-1 if unfinished)
    int core = -1; // core the job was last on (or currently on if running) (-1 if not executed yet)
    bool running = false; // true if the job is currently running
    Job(int task_id = -1, int job_id = -1, int period = 0, int release_time = 0, int exec_time = 0, int deadline = 0) : task_id(task_id), job_id(job_id), period(period), release_time(release_time), exec_time(exec_time), deadline(deadline) {}
};

struct Task {
    int phase, period, exec_time, relative_deadline;
    int next_job_id = 0;
    int next_release;
    Task(int phase, int period, int exec_time, int relative_deadline) : phase(phase), period(period), exec_time(exec_time), relative_deadline(relative_deadline), next_release(phase) {}
    Task() : Task(0, 0, 0, 0) {}
    Task(int period, int exec_time, int relative_deadline) : Task(0, period, exec_time, relative_deadline) {}
    Task(int period, int exec_time) : Task(0, period, exec_time, period) {}

    // create next job for this task (task_id = id of this task which is handled externally)
    Job next_job(int task_id);
};

typedef std::vector<Task> TaskSet;
typedef std::vector<Job> JobSet;
typedef std::vector<bool> Mask;
typedef std::vector<int> CoreState;

struct ExecBlock {
    enum EndState {PREEMPTED, COMPLETED, MISSED};
    int task_id, job_id; // task and job that executed
    int core; // core executed on
    int start; // start time
    int end; // end time
    EndState endState; // state of end
    ExecBlock(int task_id, int job_id, int core, int start, int end, EndState endState) : task_id(task_id), job_id(job_id), core(core), start(start), end(end), endState(endState) {}
};

// holds exec blocks and handles adding new ones (merges if necessary)
class ExecBlockStorage {
    bool dirty = false; // set to true when blocks added that may need to be merged
    std::deque<ExecBlock> exec_blocks;
    std::deque<ExecBlock> new_blocks;
public:
    ExecBlockStorage() {}

    // add a block to storage
    void add_block(const Job& job, int start, int end);

    // empty out storage
    void clear();

    // has new block
    bool hasNext();

    // get next new block, move it to exec_blocks
    ExecBlock getNext();
};

struct TaskSim;

struct Scheduler {
    enum PriorityScheme { STATIC, JOB_LEVEL_DYN, UNRESTRICTED_DYN };
    enum MigrationDegree { PARTITIONED, RESTRICTED, FULL};
    enum DecisionType { EVENT_BASED, QUANTUM_BASED };

    const PriorityScheme priority_scheme;
    const MigrationDegree migration_degree;
    const DecisionType decision_type;
    Scheduler(PriorityScheme priority_scheme, MigrationDegree migration_degree, DecisionType decision_type) : priority_scheme(priority_scheme), migration_degree(migration_degree), decision_type(decision_type) {}

    virtual void init(const TaskSet& task_set);

    // assign jobs to cores
    virtual CoreState schedule(const JobSet& active_jobs, int cores, int time);
};

struct TaskSim {
    TaskSet task_set;
    Scheduler* scheduler = nullptr;
    ExecBlockStorage ebs;

    int buffer = 0; // next unsimulated event time (cursor <= buffer)
    int cursor = 0; // time cursor is at
    int missed = -1; // missed job time (-1 if none)
    int cores = 1; // number of CPU cores available

    JobSet active_jobs;
    JobSet finished_jobs;

    TaskSim() {}
    
    // reset and init task sim with given task set and scheduler
    void reset(TaskSet task_set, Scheduler* scheduler, int cores);

    // simulates to at least endTime (ignore if endTime <= buffer)
    // handles execBlocks, finding next event, and updating job object bookkeeping 
    void sim(int endTime);
    
    // moves cursor to t (simulates to at least t)
    void seek(int t);
};

struct Model {
    TaskSim sim;
    Model() {}
};

#endif