#include "model.h"
#include <cassert>
#include <climits>

Job Task::next_job(int task_id) {
    Job job(task_id, next_job_id++, period, next_release, exec_time, next_release + relative_deadline);
    next_release += period;
    return job;
}

void Scheduler::init(const TaskSet& task_set) {}

CoreState Scheduler::schedule(const JobSet& active_jobs, int cores) {
    return CoreState(cores, -1);
}

void ExecBlockStorage::add_block(const Job& job, int start, int end) {
    new_blocks.emplace_back(job.task_id, job.job_id, job.core, start, end);
}

void ExecBlockStorage::clear() {
    exec_blocks.clear();
    new_blocks.clear();
}

bool ExecBlockStorage::hasNext() {
    return !new_blocks.empty();
}

ExecBlock ExecBlockStorage::getNext() {
    ExecBlock block = new_blocks.front();
    new_blocks.pop_front();
    exec_blocks.push_back(block);
    return block;
}

void TaskSim::sim(int endTime) {
    while (missed == -1 && buffer < endTime) {
        // handle job releases at buffer time
        for (int i = 0; i < task_set.size(); ++i)
            if (task_set[i].next_release == buffer)
                active_jobs.push_back(task_set[i].next_job(i));

        // sort jobs by executing first then preemptive then fresh
        int next_executing = 0;
        int next_preempted = 0;
        int next_unexecuted = 0;
        for (Job& job : active_jobs) {
            if (job.running) ++next_preempted;
            else if (job.core != -1) ++next_unexecuted;
        }
        next_unexecuted += next_preempted;
        JobSet sorted_jobs(active_jobs.size());
        for (Job& job : active_jobs) {
            if (job.running) sorted_jobs[next_executing++] = std::move(job);
            else if (job.core != -1) sorted_jobs[next_preempted++] = std::move(job);
            else sorted_jobs[next_unexecuted++] = std::move(job);
        }
        swap(sorted_jobs, active_jobs);

        // schedule
        CoreState core_state = scheduler->schedule(active_jobs, cores);
        assert(core_state.size() == cores);
        for (int i = 0; i < active_jobs.size(); ++i)
            active_jobs[i].running = false;
        for (int i = 0; i < core_state.size(); ++i) {
            if (core_state[i] != -1) {
                active_jobs[core_state[i]].core = i;
                active_jobs[core_state[i]].running = true;
            }
        }

        // find next event
        int next_event = INT_MAX;
        switch (scheduler->schedule_event) {
            case Scheduler::ScheduleEvent::JOB_RELEASE:
                // job release
                for (Task& task : task_set)
                    next_event = std::min(next_event, task.next_release);
                // job deadline or completion
                for (Job& job : active_jobs) {
                    next_event = std::min(next_event, job.deadline);
                    if (job.core != -1)
                        next_event = std::min(next_event, buffer + job.exec_time);
                }
                break;
            case Scheduler::ScheduleEvent::QUANTUM:
                next_event = buffer + 1;
                break;
        }
        int delta_time = next_event - buffer;

        // update exec blocks and buffer + handle job deadlines (and misses)
        JobSet updated_active_jobs;
        for (int i = 0; i < active_jobs.size(); ++i) {
            Job& job = active_jobs[i];
            if (job.running) {
                ebs.add_block(job, buffer, next_event);
                job.exec_time -= delta_time;
            }
            if (job.exec_time == 0)
                continue;
            if (job.deadline == next_event)
                missed = i;
            updated_active_jobs.push_back(job);
        }
        std::swap(updated_active_jobs, active_jobs);
        buffer = next_event;
    }
}

void TaskSim::seek(int t) {
    sim(t);
    cursor = std::min(t, buffer);
}

void TaskSim::reset(TaskSet task_set, Scheduler* scheduler, int cores) {
    this->task_set = task_set;
    this->scheduler = scheduler;
    this->cores = cores;
    buffer = 0;
    cursor = 0;
    missed = -1;
    active_jobs.clear();
    finished_jobs.clear();
}