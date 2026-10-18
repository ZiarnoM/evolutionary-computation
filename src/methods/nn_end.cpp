#include <limits>

#include "methods/methods.h"

Result nn_end(const Instance& inst, int start) {
    int k = inst.select_count();
    std::vector<bool> used(inst.n, false);
    Result res;
    res.sol.push_back(start);
    res.obj = inst.cost[start];
    used[start] = true;

    while ((int)res.sol.size() < k) {
        int last = res.sol.back();
        int best = -1, best_delta = std::numeric_limits<int>::max();
        for (int j = 0; j < inst.n; j++) {
            if (used[j])
                continue;
            int delta = inst.dist[last][j] + inst.cost[j];
            if (delta < best_delta) {
                best_delta = delta;
                best = j;
            }
        }
        res.sol.push_back(best);
        res.obj += best_delta;
        used[best] = true;
    }
    res.obj += inst.dist[res.sol.back()][start];
    return res;
}
