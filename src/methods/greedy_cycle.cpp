#include <limits>

#include "methods/methods.h"

Result greedy_cycle(const Instance& inst, int start) {
    int k = inst.select_count();
    const auto& d = inst.dist;
    std::vector<bool> used(inst.n, false);
    Result res;
    std::vector<int>& cyc = res.sol;
    cyc.push_back(start);
    res.obj = inst.cost[start];
    used[start] = true;

    while ((int)cyc.size() < k) {
        int m = cyc.size();
        int best = -1, best_pos = -1, best_delta = std::numeric_limits<int>::max();
        for (int j = 0; j < inst.n; j++) {
            if (used[j])
                continue;
            for (int i = 0; i < m; i++) {
                int a = cyc[i], b = cyc[(i + 1) % m];
                int delta = d[a][j] + d[j][b] - d[a][b] + inst.cost[j];
                if (delta < best_delta) {
                    best_delta = delta;
                    best = j;
                    best_pos = i + 1;
                }
            }
        }
        cyc.insert(cyc.begin() + best_pos, best);
        res.obj += best_delta;
        used[best] = true;
    }
    return res;
}
