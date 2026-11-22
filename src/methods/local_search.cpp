#include <algorithm>
#include <random>
#include <vector>

#include "methods/methods.h"

namespace {

// Moves: type 0 - replace sol[i] by the unselected node j, 1 - swap nodes at positions
// i < j, 2 - 2-opt on edges (sol[i], sol[i+1]) and (sol[j], sol[j+1]), i < j.
struct LS {
    const Instance& inst;
    Solution& sol;
    std::vector<char> used;
    int k;

    int at(int i) const { return sol[(i + k) % k]; }
    int d(int a, int b) const { return inst.dist[a][b]; }

    int delta(int type, int i, int j) const {
        if (type == 0) {
            int p = at(i - 1), v = sol[i], s = at(i + 1);
            return d(p, j) + d(j, s) - d(p, v) - d(v, s) + inst.cost[j] - inst.cost[v];
        }
        if (type == 2) {
            int a = sol[i], b = at(i + 1), c = sol[j], e = at(j + 1);
            return d(a, c) + d(b, e) - d(a, b) - d(c, e);
        }
        // neighbors: the edge between them does not change
        if (j == i + 1 || (i == 0 && j == k - 1)) {
            if (i == 0 && j == k - 1)
                std::swap(i, j);
            int p = at(i - 1), a = sol[i], b = sol[j], s = at(j + 1);
            return d(p, b) + d(a, s) - d(p, a) - d(b, s);
        }
        int pa = at(i - 1), a = sol[i], na = at(i + 1);
        int pb = at(j - 1), b = sol[j], nb = at(j + 1);
        return d(pa, b) + d(b, na) + d(pb, a) + d(a, nb) - d(pa, a) - d(a, na) - d(pb, b) - d(b, nb);
    }

    void apply(int type, int i, int j) {
        if (type == 0) {
            used[sol[i]] = 0;
            used[j] = 1;
            sol[i] = j;
        } else if (type == 1) {
            std::swap(sol[i], sol[j]);
        } else {
            std::reverse(sol.begin() + i + 1, sol.begin() + j + 1);
        }
    }

    bool valid_2opt(int i, int j) const { return j > i + 1 && !(i == 0 && j == k - 1); }
};

}  // namespace

Result local_search(const Instance& inst, int start, bool steepest, bool edges, bool greedy_start) {
    Result res;
    if (greedy_start) {
        // best of all construction heuristics from the same starting node
        std::vector<Result> starts = {nn_end(inst, start),
                                      nn_any(inst, start),
                                      greedy_cycle(inst, start),
                                      regret_insertion(inst, start, true, 0.0),
                                      regret_insertion(inst, start, true, 1.0),
                                      nn_any_regret(inst, start),
                                      nn_any_weighted(inst, start),
                                      greedy_cycle_regret(inst, start),
                                      greedy_cycle_weighted(inst, start)};
        res = *std::min_element(starts.begin(), starts.end(),
                                [](const Result& a, const Result& b) { return a.obj < b.obj; });
    } else {
        res = random_solution(inst, start);
    }

    int n = inst.n;
    LS ls{inst, res.sol, std::vector<char>(n, 0), (int)res.sol.size()};
    for (int v : res.sol)
        ls.used[v] = 1;
    int k = ls.k, intra = edges ? 2 : 1;
    std::mt19937 rng(42 + start);

    while (true) {
        int best = 0, bt = -1, bi = 0, bj = 0;
        if (steepest) {
            // same deltas as in LS::delta, written out so the parts fixed for i are computed once
            auto& D = inst.dist;
            for (int i = 0; i < k; i++) {
                int p = ls.at(i - 1), v = res.sol[i], s = ls.at(i + 1);
                int base = -D[p][v] - D[v][s] - inst.cost[v];
                for (int j = 0; j < n; j++) {
                    if (ls.used[j])
                        continue;
                    int x = base + D[p][j] + D[j][s] + inst.cost[j];
                    if (x < best)
                        best = x, bt = 0, bi = i, bj = j;
                }
            }
            for (int i = 0; i < k; i++) {
                if (!edges) {
                    for (int j = i + 1; j < k; j++) {
                        int x = ls.delta(1, i, j);
                        if (x < best)
                            best = x, bt = 1, bi = i, bj = j;
                    }
                    continue;
                }
                int a = res.sol[i], b = ls.at(i + 1), ab = D[a][b];
                for (int j = i + 2; j < k - (i == 0); j++) {
                    int c = res.sol[j], e = ls.at(j + 1);
                    int x = D[a][c] + D[b][e] - ab - D[c][e];
                    if (x < best)
                        best = x, bt = 2, bi = i, bj = j;
                }
            }
        } else {
            // greedy: random order of the two kinds of moves and random starting
            // points of both loops, the first improving move is taken
            bool inter_first = rng() % 2;
            int oi = rng() % k, oj = rng() % n;
            for (int part = 0; part < 2 && bt < 0; part++) {
                if ((part == 0) == inter_first) {
                    for (int a = 0; a < k && bt < 0; a++)
                        for (int b = 0; b < n; b++) {
                            int i = (oi + a) % k, j = (oj + b) % n;
                            if (ls.used[j])
                                continue;
                            int x = ls.delta(0, i, j);
                            if (x < 0) {
                                best = x, bt = 0, bi = i, bj = j;
                                break;
                            }
                        }
                } else {
                    for (int a = 0; a < k && bt < 0; a++)
                        for (int b = 0; b < k; b++) {
                            int i = (oi + a) % k, j = (oj + b) % k;
                            if (i > j)
                                std::swap(i, j);
                            if (i == j || (edges && !ls.valid_2opt(i, j)))
                                continue;
                            int x = ls.delta(intra, i, j);
                            if (x < 0) {
                                best = x, bt = intra, bi = i, bj = j;
                                break;
                            }
                        }
                }
            }
        }
        if (bt < 0)
            break;
        ls.apply(bt, bi, bj);
        res.obj += best;
    }
    return res;
}

Result steepest_nodes_random(const Instance& inst, int start) { return local_search(inst, start, true, false, false); }
Result steepest_nodes_greedy(const Instance& inst, int start) { return local_search(inst, start, true, false, true); }
Result steepest_edges_random(const Instance& inst, int start) { return local_search(inst, start, true, true, false); }
Result steepest_edges_greedy(const Instance& inst, int start) { return local_search(inst, start, true, true, true); }
Result greedy_nodes_random(const Instance& inst, int start) { return local_search(inst, start, false, false, false); }
Result greedy_nodes_greedy(const Instance& inst, int start) { return local_search(inst, start, false, false, true); }
Result greedy_edges_random(const Instance& inst, int start) { return local_search(inst, start, false, true, false); }
Result greedy_edges_greedy(const Instance& inst, int start) { return local_search(inst, start, false, true, true); }
