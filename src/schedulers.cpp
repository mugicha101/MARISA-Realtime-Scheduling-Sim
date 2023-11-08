#include "schedulers.h"

#include <vector>
#include <numeric>

CoreState TEMP::schedule(const JobSet& active_jobs, int cores) {
    CoreState core_state(cores, -1);
    int cap = std::min(cores, (int)active_jobs.size());
    for (int i = 0; i < cap; ++i)
        core_state[i] = i;
    return core_state;
}