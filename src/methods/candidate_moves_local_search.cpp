#include <algorithm>
#include <vector>

#include "methods/methods.h"

// Steepest local search (2-opt + node exchange, random start) that only evaluates moves
// adding at least one candidate edge. Candidates of a node are its nearest nodes. Ranking them
// by distance plus node cost was worse on TSPA, where the costs are large and the list was
// filled with cheap but distant nodes.
Result steepest_edges_random_candidate(const Instance& inst, int start, int candidate_count) {
    int n = inst.n;
    auto& D = inst.dist;
    std::vector<std::vector<int>> cand(n);
    for (int a = 0; a < n; a++) {
        std::vector<int> o;
        for (int b = 0; b < n; b++)
            if (b != a)
                o.push_back(b);
        int m = std::min(candidate_count, n - 1);
        std::partial_sort(o.begin(), o.begin() + m, o.end(), [&](int x, int y) { return D[a][x] < D[a][y]; });
        o.resize(m);
        cand[a] = o;
    }

    Result res = random_solution(inst, start);
    Solution& sol = res.sol;
    int k = sol.size();
    std::vector<int> pos(n, -1);
    for (int i = 0; i < k; i++)
        pos[sol[i]] = i;
    auto at = [&](int i) { return sol[(i + k) % k]; };
    // 2-opt on edges starting at positions i and j: adds edges (sol[i], sol[j]) and (sol[i+1], sol[j+1])
    auto two_opt = [&](int i, int j) {
        int a = sol[i], b = at(i + 1), c = sol[j], d = at(j + 1);
        return D[a][c] + D[b][d] - D[a][b] - D[c][d];
    };
    // replace the node at position i by the unselected node u
    auto exchange = [&](int i, int u) {
        int p = at(i - 1), v = sol[i], s = at(i + 1);
        return D[p][u] + D[u][s] - D[p][v] - D[v][s] + inst.cost[u] - inst.cost[v];
    };

    while (true) {
        int best = 0, bt = -1, bi = 0, bj = 0;
        auto take = [&](int x, int t, int i, int j) {
            if (x < best)
                best = x, bt = t, bi = i, bj = j;
        };
        for (int i = 0; i < k; i++) {
            int a = sol[i];
            for (int b : cand[a]) {
                if (pos[b] >= 0) {
                    int j = pos[b];
                    // a->a+1, b->b+1 or a-1->a, b-1->b, both add the edge a-b
                    if (at(i + 1) != b && at(j + 1) != a)
                        take(two_opt(i, j), 0, i, j);
                    int pi = (i + k - 1) % k, pj = (j + k - 1) % k;
                    if (at(i - 1) != b && at(j - 1) != a)
                        take(two_opt(pi, pj), 0, pi, pj);
                } else {
                    // b replaces the successor or the predecessor of a
                    take(exchange((i + 1) % k, b), 1, (i + 1) % k, b);
                    take(exchange((i + k - 1) % k, b), 1, (i + k - 1) % k, b);
                }
            }
        }
        if (bt < 0)
            break;
        if (bt == 1) {
            pos[sol[bi]] = -1;
            sol[bi] = bj;
            pos[bj] = bi;
        } else {
            int i = std::min(bi, bj), j = std::max(bi, bj);
            std::reverse(sol.begin() + i + 1, sol.begin() + j + 1);
            for (int t = i + 1; t <= j; t++)
                pos[sol[t]] = t;
        }
        res.obj += best;
    }
    return res;
}

Result steepest_edges_random_10candidate(const Instance& inst, int start) {
    return steepest_edges_random_candidate(inst, start, 10);
}
