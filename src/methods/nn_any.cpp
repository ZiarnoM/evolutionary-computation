#include <limits>

#include "methods/methods.h"

// Builds an open path, the closing edge is only added at the end, so inserting
// at the front or back costs a single edge.
Result nn_any(const Instance& inst, int start) {
    int k = inst.select_count();
    const auto& d = inst.dist;
    std::vector<bool> used(inst.n, false);
    Result res;
    std::vector<int>& path = res.sol;
    path.push_back(start);
    res.obj = inst.cost[start];
    used[start] = true;

    while ((int)path.size() < k) {
        int m = path.size();
        int best = -1, best_pos = -1, best_delta = std::numeric_limits<int>::max();
        for (int j = 0; j < inst.n; j++) {
            if (used[j])
                continue;
            // pos p means j goes before path[p], p == m means after the last node
            for (int p = 0; p <= m; p++) {
                int delta;
                if (p == 0)
                    delta = d[j][path[0]];
                else if (p == m)
                    delta = d[path[m - 1]][j];
                else
                    delta = d[path[p - 1]][j] + d[j][path[p]] - d[path[p - 1]][path[p]];
                delta += inst.cost[j];
                if (delta < best_delta) {
                    best_delta = delta;
                    best = j;
                    best_pos = p;
                }
            }
        }
        path.insert(path.begin() + best_pos, best);
        res.obj += best_delta;
        used[best] = true;
    }
    res.obj += d[path.back()][path.front()];
    return res;
}
