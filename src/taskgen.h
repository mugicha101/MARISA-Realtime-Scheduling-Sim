#include "model.h"
#include <random>

#ifndef TASKGEN_H
#define TASKGEN_H

struct TaskSetGenerator {
    static std::default_random_engine gen;
    TaskSetGenerator() = delete;

    // generates a periodic synchronous implicit-deadline task set t of size <task_count> using discrete time of length <1/precision>
    // all time fractions need a denominator divisible by <precision>
    // for each task, uniformly chooses a period from the closed-interval [min_period, max_period]
    // if input invalid, returns an empty set

    // uses modified Kraemer Algorithm defined here https://www.cs.cmu.edu/~nasmith/papers/smith+tromble.tr04.pdf
    static TaskSet genModifiedKraemer(int precision, Fraction util, int task_count, Fraction min_period, Fraction max_period);

    // uses UUniFast-Discard
    static TaskSet genUUniFastDiscard(int precision, Fraction util, int task_count, Fraction min_period, Fraction max_period);

    // both genURPartition and genUUniFastDiscard should be indistinguishable, but genUUniFastDiscard is the formalized method
};

#endif