#pragma once

#include <functional>
#include <string>

#include "core/instance.h"
#include "core/solution.h"

// obj is the objective as tracked by the method itself, checked against
// a full recomputation in the experiment.
struct Result {
    Solution sol;
    int obj = 0;
};

using MethodFn = std::function<Result(const Instance& inst, int start)>;

struct Method {
    std::string name;
    MethodFn run;
};

Result random_solution(const Instance& inst, int start);
Result nn_end(const Instance& inst, int start);
Result nn_any(const Instance& inst, int start);
Result greedy_cycle(const Instance& inst, int start);

// w = 0 gives the plain greedy choice, w = 1 the pure 2-regret, see regret.cpp
Result regret_insertion(const Instance& inst, int start, bool cycle, double w);
Result nn_any_regret(const Instance& inst, int start);
Result nn_any_weighted(const Instance& inst, int start);
Result greedy_cycle_regret(const Instance& inst, int start);
Result greedy_cycle_weighted(const Instance& inst, int start);
