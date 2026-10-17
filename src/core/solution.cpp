#include "core/solution.h"

int cycle_length(const Instance& inst, const Solution& sol) {
    int len = 0;
    int m = sol.size();
    for (int i = 0; i < m; i++)
        len += inst.dist[sol[i]][sol[(i + 1) % m]];
    return len;
}

int nodes_cost(const Instance& inst, const Solution& sol) {
    int sum = 0;
    for (int v : sol)
        sum += inst.cost[v];
    return sum;
}

int objective(const Instance& inst, const Solution& sol) {
    return cycle_length(inst, sol) + nodes_cost(inst, sol);
}

std::string validate(const Instance& inst, const Solution& sol) {
    if ((int)sol.size() != inst.select_count())
        return "expected " + std::to_string(inst.select_count()) + " nodes, got " +
               std::to_string(sol.size());
    std::vector<bool> used(inst.n, false);
    for (int v : sol) {
        if (v < 0 || v >= inst.n)
            return "index out of range: " + std::to_string(v);
        if (used[v])
            return "duplicate node: " + std::to_string(v);
        used[v] = true;
    }
    return "";
}
