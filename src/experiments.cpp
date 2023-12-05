#ifndef EXPERIMENTS_CPP
#define EXPERIMENTS_CPP

#include "model.h"
#include "taskgen.h"
#include "schedulers/schedulers.h"

#include <vector>
#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <deque>

struct SchedulerData {
    std::vector<Fraction> util_data;
    std::vector<double> schedulability_data;
    std::vector<double> cswitch_data;
    std::vector<double> mig_data;
};

class Experiment {
public:
    static void kraemer() {
        const int TRIALS = 1000;
        const int PRECISION = 1000;
        const int DIM = 3;
        std::ofstream output;
        output.open("experiment_data_kraemer.txt");
        for (int i = 0; i < TRIALS; ++i) {
            TaskSet task_set = TaskSetGenerator::genModifiedKraemer(PRECISION, Fraction(3,2), DIM, 1, 1);
            output << "(" << *(task_set[0].exec_time);
            for (int j = 1; j < DIM; ++j) {
                output << "," << *(task_set[j].exec_time);
            }
            output << ")";
        }
        output.close();
    }

    static void sched(int cores) {
        const int UTIL_STEPS = 200;
        const int PRECISION = UTIL_STEPS * 1000;
        const int TRIALS_PER_UTIL = 50;
        const int TASK_COUNT = 12;
        const int MIN_PERIOD = 4;
        const int MAX_PERIOD = 12;
        const int SIM_TIME = 1000;
        const int SCHED_COUNT = 4;
        const int PD2_SCALE = 10;

        std::cout << "SETTING UP EXPERIMENT" << std::endl;
        Scheduler* schedulers[SCHED_COUNT];
        std::string scheduler_names[SCHED_COUNT];
        Fraction sched_check_util[SCHED_COUNT];

        schedulers[0] = new GEDF(true);
        scheduler_names[0] = "GEDF";
        sched_check_util[0] = Fraction(0,2) * cores;

        schedulers[1] = new EDZL();
        scheduler_names[1] = "EDZL";
        sched_check_util[1] = Fraction(3,4) * cores;

        schedulers[2] = new PD2(true);
        scheduler_names[2] = "PD2";
        sched_check_util[2] = Fraction(1,2) * cores;

        schedulers[3] = new LLREF();
        scheduler_names[3] = "LLREF";
        sched_check_util[3] = Fraction(1,1) * cores;
        SchedulerData data[SCHED_COUNT];
        for (int i = 0; i < 4; ++i)
            data[i] = SchedulerData();
        Fraction step = Fraction(cores, UTIL_STEPS);
        SimModel model;
        model.ebs_active = false;
        int max_lcm = 1;
        for (int p = MIN_PERIOD; p <= MAX_PERIOD; ++p) {
            max_lcm = std::lcm(max_lcm, p);
        }
        std::vector<std::vector<float>> sample_points;
        std::cout << "MAX LCM: " << max_lcm << std::endl;

        std::cout << "RUNNING EXPERIMENT" << std::endl;
        for (Fraction util = step; util <= cores; util += step) {
            std::cout << "UTIL " << *util << std::endl;
            long long schedulable_count[SCHED_COUNT];
            long long cswitch_count[SCHED_COUNT];
            long long mig_count[SCHED_COUNT];
            for (int i = 0; i < SCHED_COUNT; ++i) {
                schedulable_count[i] = 0;
                cswitch_count[i] = 0;
                mig_count[i] = 0;
            }
            for (int trial = 0; trial < TRIALS_PER_UTIL; ++trial) {
                std::cout << "TRIAL " << trial << std::endl;
                TaskSet task_set = TaskSetGenerator::genModifiedKraemer(PRECISION, util, TASK_COUNT, MIN_PERIOD, MAX_PERIOD);
                sample_points.emplace_back();
                for (int i = 0; i < TASK_COUNT; ++i)
                    sample_points.back().push_back(*(task_set[i].exec_time / task_set[i].period));
                long long hyperperiod = 1;
                for (Task& task : model.task_set) {
                    assert(task.period.isInt());
                    hyperperiod = std::lcm(hyperperiod, task.period.getNum());
                }
                for (int i = 0; i < SCHED_COUNT; ++i) {
                    std::cout << scheduler_names[i] << std::endl;
                    // init model
                    long long h = hyperperiod;
                    long long cmp_time = SIM_TIME;
                    if (i == 2) {
                        cmp_time *= PD2_SCALE;
                        // handle PD2 task set discretizing
                        TaskSet pd2_task_set;
                        pd2_task_set.reserve(task_set.size());
                        for (Task task : task_set) {
                            task.exec_time = (task.exec_time * PD2_SCALE).ceil();
                            task.period = task.period * PD2_SCALE;
                            task.relative_deadline = task.period;
                            pd2_task_set.push_back(task);
                        }
                        model.reset(pd2_task_set, schedulers[i], cores);
                        h *= PD2_SCALE;
                    } else model.reset(task_set, schedulers[i], cores);
                    
                    // simulate to sim time, count cswitch and mig counts of schedulable tasks
                    model.sim(cmp_time);
                    if (model.missed != -1) continue;
                    long long cswitches = model.cswitch_count;
                    long long migs = 0;
                    for (Job& job : model.finished_jobs)
                        migs += job.migration_count;
                    for (Job& job : model.active_jobs)
                        migs += job.migration_count;

                    // simulate to 2H to check for schedulability
                    if (util > sched_check_util[i]) {
                        model.sim(h * 2);
                        std::cout << "SCHED CHECK t=" << (2 * h) << ": " << (model.missed == -1) << std::endl;
                    }
                    if (model.missed != -1) continue;
                    ++schedulable_count[i];
                    cswitch_count[i] += cswitches;
                    mig_count[i] += migs;
                }
            }
            for (int i = 0; i < SCHED_COUNT; ++i) {
                data[i].util_data.push_back(util);
                data[i].schedulability_data.push_back((double)schedulable_count[i] / (double)TRIALS_PER_UTIL);
                data[i].cswitch_data.push_back(schedulable_count[i] == 0 ? 0 : (double)cswitch_count[i] / (double)schedulable_count[i]);
                data[i].mig_data.push_back(schedulable_count[i] == 0 ? 0 : (double)mig_count[i] / (double)schedulable_count[i]);
            }
        }

        // write to file
        std::cout << "OUTPUTING" << std::endl;
        std::ofstream output;
        output.open("experiment_data_" + std::to_string(cores) + "cores.txt");
        for (int i = 0; i < SCHED_COUNT; ++i) {
            output << scheduler_names[i] << std::endl;

            // schedulability data
            output << "sched: ";
            for (int j = 0; j < data[i].util_data.size(); ++j)
                output << "(" << *data[i].util_data[j] << "," << data[i].schedulability_data[j] << ")";
            output << std::endl;

            // context switch data
            output << "cswitch: ";
            for (int j = 0; j < data[i].util_data.size(); ++j)
                output << "(" << *data[i].util_data[j] << "," << data[i].cswitch_data[j] << ")";
            output << std::endl;

            // migration data
            output << "migrations: ";
            for (int j = 0; j < data[i].util_data.size(); ++j)
                output << "(" << *data[i].util_data[j] << "," << data[i].mig_data[j] << ")";
            output << std::endl;
        }

        // sample points
        output << "sample points: ";
        for (auto& p : sample_points) {
            output << "(" << p[0];
            for (int i = 1; i < TASK_COUNT; ++i)
                output << "," << p[i];
            output << ")" << std::endl;
        }
        output.close();
        std::cout << "EXPERIMENT DONE" << std::endl;
    }
public:
};

#endif