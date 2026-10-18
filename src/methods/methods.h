#pragma once

#include <string>
#include <vector>

#include "core/instance.h"
#include "core/solution.h"

// obj is the objective as tracked by the method itself, checked against
// a full recomputation in the experiment.
struct Result {
    Solution sol;
    int obj = 0;
};

using MethodFn = Result (*)(const Instance& inst, int start);

struct Method {
    std::string name;
    MethodFn run;
};

Result random_solution(const Instance& inst, int start);
