#include <limits>

#include "methods/methods.h"

// Same moves as nn_any (cycle = false) or greedy_cycle (cycle = true), but for every
// unselected node the best and the second best insertion are found and the node with the
// highest w * regret - (1 - w) * best is inserted at its best position. Ties go to the
// node with the smaller best insertion, then to the lower index.
Result regret_insertion(const Instance& inst, int start, bool cycle, double w) {
    const int inf = std::numeric_limits<int>::max();
    int k = inst.select_count();
    const auto& d = inst.dist;
    std::vector<bool> used(inst.n, false);
    Result res;
    std::vector<int>& sol = res.sol;
    sol.push_back(start);
    res.obj = inst.cost[start];
    used[start] = true;

    while ((int)sol.size() < k) {
        int m = sol.size();
        int npos = cycle ? m : m + 1;
        int best = -1, best_pos = -1, best_delta = 0;
        double best_score = 0;
        for (int j = 0; j < inst.n; j++) {
            if (used[j])
                continue;
            int d1 = inf, d2 = inf, pos = -1;
            for (int p = 0; p < npos; p++) {
                // cycle: j goes into the edge (sol[p], sol[p + 1]), path: before sol[p]
                int delta;
                if (cycle) {
                    int a = sol[p], b = sol[(p + 1) % m];
                    delta = d[a][j] + d[j][b] - d[a][b];
                } else if (p == 0) {
                    delta = d[j][sol[0]];
                } else if (p == m) {
                    delta = d[sol[m - 1]][j];
                } else {
                    delta = d[sol[p - 1]][j] + d[j][sol[p]] - d[sol[p - 1]][sol[p]];
                }
                delta += inst.cost[j];
                if (delta < d1) {
                    d2 = d1;
                    d1 = delta;
                    pos = cycle ? p + 1 : p;
                } else if (delta < d2) {
                    d2 = delta;
                }
            }
            // a cycle of one node has only one position
            int regret = d2 == inf ? 0 : d2 - d1;
            double score = w * regret - (1 - w) * d1;
            if (best < 0 || score > best_score || (score == best_score && d1 < best_delta)) {
                best = j;
                best_pos = pos;
                best_delta = d1;
                best_score = score;
            }
        }
        sol.insert(sol.begin() + best_pos, best);
        res.obj += best_delta;
        used[best] = true;
    }
    if (!cycle)
        res.obj += d[sol.back()][sol.front()];
    return res;
}

Result nn_any_regret(const Instance& inst, int start) {
    return regret_insertion(inst, start, false, 1);
}

Result nn_any_weighted(const Instance& inst, int start) {
    return regret_insertion(inst, start, false, 0.5);
}

Result greedy_cycle_regret(const Instance& inst, int start) {
    return regret_insertion(inst, start, true, 1);
}

Result greedy_cycle_weighted(const Instance& inst, int start) {
    return regret_insertion(inst, start, true, 0.5);
}
