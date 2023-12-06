#include "schedulers.h"

void PD2::init(const TaskSet& task_set, int cores) {
    valid_task_set = usesIntegerTime(task_set);
}

ScheduleDecision PD2::schedule(const SimModel& model) {
    ScheduleDecision sd(model.cores);
    if (!valid_task_set) return sd; // don't schedule if tasks don't use integer time
    auto priority_func = [early_release = this->early_release, &time = model.time](const Job& job) {
        auto get_itv = [&job](int work_done) {
            int rel_deadline = job.deadline.getNum() - job.release_time.getNum();
            return std::make_pair(
                (int)job.release_time.getNum() + std::max(0, ((work_done - 1) * rel_deadline + (int)job.exec_time.getNum()) / (int)job.exec_time.getNum() - 1),
                (int)job.release_time.getNum() + std::min(rel_deadline - 1, (work_done * rel_deadline + (int)job.exec_time.getNum() - 1) / (int)job.exec_time.getNum() - 1)
            );
        };
        std::pair<int,int> first_itv = get_itv((int)job.runtime.getNum() + 1);

        // handle early releasing
        if (early_release)
            first_itv.first = 0;
        else if (first_itv.first > time)
            return (long long)-1;

        // generate intervals to next group deadline
        int curr_work = job.runtime.getNum();
        std::pair<int,int> curr_itv, next_itv = first_itv;
        bool overlapping_next;
        int curr_itv_len;
        auto step_itv = [&]() {
            curr_itv = next_itv;
            next_itv = get_itv(++curr_work + 1);
            overlapping_next = curr_itv.second == next_itv.first;
            curr_itv_len = curr_itv.second + 1 - curr_itv.first;
        };
        step_itv();

        // priority order: deadline, is heavy, itv overlaps next, next group deadline
        // priority bitstring (64 bits):
        //   32b - deadline
        //    1b - first interval overlaps next
        //   31b - next group deadline
        long long priority = (long long)(INT_MAX - first_itv.second) << 32; // deadline of current interval
        if (overlapping_next) // first itv overlapping next
            priority += (long long)1 << 31;

        while (curr_work < job.exec_time && overlapping_next && curr_itv_len == 2)
            step_itv();
        priority += curr_itv.first + 1; // next group deadline
        return priority;
    };
    assignToCores(model.active_jobs, sd.core_state, chooseByPriority<long long>(model.active_jobs, model.cores, -1, priority_func));
    sd.next_event = model.time + 1;
    return sd;
}