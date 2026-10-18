#include <algorithm>
#include <numeric>
#include <random>

#include "methods/methods.h"

Result random_solution(const Instance& inst, int start) {
    // start is only used to seed the generator, so every run can be repeated on its own
    std::mt19937 rng(42 + start);
    std::vector<int> nodes(inst.n);
    std::iota(nodes.begin(), nodes.end(), 0);
    std::shuffle(nodes.begin(), nodes.end(), rng);

    Result res;
    res.sol.assign(nodes.begin(), nodes.begin() + inst.select_count());
    res.obj = objective(inst, res.sol);
    return res;
}
