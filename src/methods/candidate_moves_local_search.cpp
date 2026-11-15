#include <algorithm>
#include <numeric>
#include <random>
#include <set>
#include <tuple>
#include <vector>

#include "methods/methods.h"

namespace
{
    enum class SearchType
    {
        STEEPEST,
        GREEDY
    };

    enum class IntraMove
    {
        NODES,
        EDGES
    };

    enum class MoveType
    {
        INTER,
        INTRA_NODES,
        INTRA_EDGES
    };

    struct Move
    {
        MoveType type;
        int i;
        int j;
    };

    using CandidateLists = std::vector<std::vector<int>>;

    // Build k nearest neighbors for every node.
    // Intra-route: score = distance(i,j).
    // Inter-route: score = distance(i,j) + cost[j], i.e. cost of the
    // potential replacement node is included.
    CandidateLists build_candidates(
        const Instance &inst,
        int candidate_count,
        bool include_node_cost)
    {
        CandidateLists candidates(inst.n);

        for (int i = 0; i < inst.n; ++i)
        {
            std::vector<std::pair<int, int>> ranked;
            ranked.reserve(inst.n - 1);

            for (int j = 0; j < inst.n; ++j)
            {
                if (i == j)
                    continue;

                int score = inst.dist[i][j];

                if (include_node_cost)
                    score += inst.cost[j];

                ranked.push_back({score, j});
            }

            std::sort(ranked.begin(), ranked.end());

            int count = std::min(
                candidate_count,
                static_cast<int>(ranked.size()));

            for (int t = 0; t < count; ++t)
                candidates[i].push_back(ranked[t].second);
        }

        return candidates;
    }

    // Candidate edges are treated as undirected:
    // an edge is a candidate if either endpoint lists the other.
    bool is_candidate_edge(
        const CandidateLists &candidates,
        int a,
        int b)
    {
        const auto contains = [](const std::vector<int> &list, int node)
        {
            return std::find(list.begin(), list.end(), node) != list.end();
        };

        return contains(candidates[a], b) ||
               contains(candidates[b], a);
    }

    int nodes_exchange_delta(
        const Instance &inst,
        const Solution &sol,
        int i,
        int j)
    {
        int k = static_cast<int>(sol.size());

        std::vector<int> edges = {
            (i - 1 + k) % k,
            i,
            (j - 1 + k) % k,
            j};

        std::sort(edges.begin(), edges.end());
        edges.erase(
            std::unique(edges.begin(), edges.end()),
            edges.end());

        auto vertex = [&](int pos)
        {
            if (pos == i)
                return sol[j];
            if (pos == j)
                return sol[i];
            return sol[pos];
        };

        int delta = 0;

        for (int e : edges)
        {
            int next = (e + 1) % k;
            delta -= inst.dist[sol[e]][sol[next]];
            delta += inst.dist[vertex(e)][vertex(next)];
        }

        return delta;
    }

    int edges_exchange_delta(
        const Instance &inst,
        const Solution &sol,
        int i,
        int j)
    {
        int k = static_cast<int>(sol.size());

        int a = sol[i];
        int b = sol[(i + 1) % k];
        int c = sol[j];
        int d = sol[(j + 1) % k];

        return inst.dist[a][c] + inst.dist[b][d] -
               inst.dist[a][b] - inst.dist[c][d];
    }

    int inter_exchange_delta(
        const Instance &inst,
        const Solution &sol,
        int i,
        int candidate)
    {
        int k = static_cast<int>(sol.size());

        int prev = sol[(i - 1 + k) % k];
        int curr = sol[i];
        int next = sol[(i + 1) % k];

        return inst.dist[prev][candidate] +
               inst.dist[candidate][next] -
               inst.dist[prev][curr] -
               inst.dist[curr][next] +
               inst.cost[candidate] -
               inst.cost[curr];
    }

    void apply_move(
        const Move &move,
        Solution &sol,
        std::vector<bool> &used)
    {
        if (move.type == MoveType::INTER)
        {
            int removed = sol[move.i];
            int inserted = move.j;

            sol[move.i] = inserted;
            used[removed] = false;
            used[inserted] = true;
        }
        else if (move.type == MoveType::INTRA_NODES)
        {
            std::swap(sol[move.i], sol[move.j]);
        }
        else
        {
            std::reverse(
                sol.begin() + move.i + 1,
                sol.begin() + move.j + 1);
        }
    }

    // Add a 2-opt move only if it is valid and introduces the given
    // candidate edge. Canonicalizing the cut-edge indices prevents
    // duplicate representations of the same 2-opt move.
    void add_two_opt_candidate(
        int cut_a,
        int cut_b,
        int k,
        std::set<std::tuple<int, int, int>> &seen,
        std::vector<Move> &moves)
    {
        int i = std::min(cut_a, cut_b);
        int j = std::max(cut_a, cut_b);

        // Adjacent cut edges and the cycle-boundary pair do not define
        // a valid nontrivial 2-opt move.
        if (j == i + 1 || (i == 0 && j == k - 1))
            return;

        auto key = std::make_tuple(
            static_cast<int>(MoveType::INTRA_EDGES), i, j);

        if (seen.insert(key).second)
            moves.push_back({MoveType::INTRA_EDGES, i, j});
    }

    // Generate only candidate moves. No complete neighborhood is built.
    std::vector<Move> generate_candidate_moves(
        const Instance &inst,
        const Solution &sol,
        const std::vector<bool> &used,
        IntraMove intra,
        const CandidateLists &intra_candidates,
        const CandidateLists &inter_candidates)
    {
        const int n = inst.n;
        const int k = static_cast<int>(sol.size());

        std::vector<Move> moves;
        std::set<std::tuple<int, int, int>> seen;

        std::vector<int> position(n, -1);
        for (int i = 0; i < k; ++i)
            position[sol[i]] = i;

        // Inter-route candidate moves.
        // A replacement y is generated from candidate lists of the
        // predecessor or successor of the selected node x. Therefore
        // at least one introduced edge (prev,y) or (y,next) is a
        // candidate edge.
        for (int i = 0; i < k; ++i)
        {
            int prev = sol[(i - 1 + k) % k];
            int next = sol[(i + 1) % k];

            auto add_inter_from_endpoint = [&](int endpoint)
            {
                for (int candidate : inter_candidates[endpoint])
                {
                    if (used[candidate])
                        continue;

                    if (!is_candidate_edge(
                            inter_candidates, endpoint, candidate))
                        continue;

                    auto key = std::make_tuple(
                        static_cast<int>(MoveType::INTER), i, candidate);

                    if (seen.insert(key).second)
                        moves.push_back({MoveType::INTER, i, candidate});
                }
            };

            add_inter_from_endpoint(prev);
            add_inter_from_endpoint(next);
        }

        if (intra == IntraMove::EDGES)
        {
            // For every selected node a and each of its candidate
            // neighbors b, generate the two 2-opt moves that introduce
            // edge (a,b):
            //  1) cut (pred(a),a) and (b,succ(b));
            //  2) cut (a,succ(a)) and (pred(b),b).
            for (int a = 0; a < n; ++a)
            {
                if (!used[a])
                    continue;

                int pa = position[a];

                for (int b : intra_candidates[a])
                {
                    if (!used[b] || a == b)
                        continue;

                    int pb = position[b];

                    int pred_a = (pa - 1 + k) % k;
                    int pred_b = (pb - 1 + k) % k;

                    add_two_opt_candidate(pa, pb, k, seen, moves);
                    add_two_opt_candidate(pred_a, pred_b, k, seen, moves);
                }
            }
        }
        else
        {
            // Node-swap candidate moves.
            // A swap of selected nodes x and y introduces edges between
            // y and the old neighbors of x, and between x and the old
            // neighbors of y. Generate swaps from candidate lists of
            // the neighboring vertices, then verify the actual new
            // edges before adding the move.
            for (int i = 0; i < k; ++i)
            {
                int x = sol[i];
                int prev = sol[(i - 1 + k) % k];
                int next = sol[(i + 1) % k];

                auto add_swaps_from_endpoint = [&](int endpoint)
                {
                    for (int y : intra_candidates[endpoint])
                    {
                        if (!used[y] || y == x)
                            continue;

                        int j = position[y];

                        // Check all edges introduced by swapping x and y.
                        int y_prev = sol[(j - 1 + k) % k];
                        int y_next = sol[(j + 1) % k];

                        bool introduces_candidate =
                            is_candidate_edge(intra_candidates, prev, y) ||
                            is_candidate_edge(intra_candidates, next, y) ||
                            is_candidate_edge(intra_candidates, y_prev, x) ||
                            is_candidate_edge(intra_candidates, y_next, x);

                        if (!introduces_candidate)
                            continue;

                        int first = std::min(i, j);
                        int second = std::max(i, j);

                        auto key = std::make_tuple(
                            static_cast<int>(MoveType::INTRA_NODES),
                            first, second);

                        if (seen.insert(key).second)
                            moves.push_back({MoveType::INTRA_NODES, first, second});
                    }
                };

                add_swaps_from_endpoint(prev);
                add_swaps_from_endpoint(next);
            }
        }

        return moves;
    }
} // namespace

Result local_search_candidate(
    const Instance &inst,
    int start,
    SearchType search,
    IntraMove intra,
    bool use_greedy_start,
    int candidate_count)
{
    int n = inst.n;
    int k = inst.select_count();

    Result res;

    if (use_greedy_start)
    {
        std::vector<Result> candidates = {
            nn_end(inst, start),
            nn_any(inst, start),
            greedy_cycle(inst, start),
            regret_insertion(inst, start, true, 0.0),
            regret_insertion(inst, start, true, 1.0),
            nn_any_regret(inst, start),
            nn_any_weighted(inst, start),
            greedy_cycle_regret(inst, start),
            greedy_cycle_weighted(inst, start)};

        res = *std::min_element(
            candidates.begin(),
            candidates.end(),
            [](const Result &a, const Result &b)
            {
                return a.obj < b.obj;
            });
    }
    else
    {
        res = random_solution(inst, start);
    }

    std::vector<bool> used(n, false);
    for (int v : res.sol)
        used[v] = true;

    // Candidate lists are built once and reused throughout the search.
    // Intra-route rankings ignore node costs; inter-route rankings
    // include the cost of the potential replacement node.
    const CandidateLists intra_candidates =
        build_candidates(inst, candidate_count, false);

    const CandidateLists inter_candidates =
        build_candidates(inst, candidate_count, true);

    std::mt19937 rng(42 + start);

    while (true)
    {
        std::vector<Move> moves = generate_candidate_moves(
            inst, res.sol, used, intra,
            intra_candidates, inter_candidates);

        if (search == SearchType::GREEDY)
            std::shuffle(moves.begin(), moves.end(), rng);

        bool found = false;
        int best_delta = 0;
        Move best_move{};

        for (const Move &move : moves)
        {
            int delta;

            if (move.type == MoveType::INTER)
            {
                delta = inter_exchange_delta(
                    inst, res.sol, move.i, move.j);
            }
            else if (move.type == MoveType::INTRA_NODES)
            {
                delta = nodes_exchange_delta(
                    inst, res.sol, move.i, move.j);
            }
            else
            {
                delta = edges_exchange_delta(
                    inst, res.sol, move.i, move.j);
            }

            if (delta >= 0)
                continue;

            if (search == SearchType::GREEDY)
            {
                best_move = move;
                best_delta = delta;
                found = true;
                break;
            }

            if (!found || delta < best_delta)
            {
                best_delta = delta;
                best_move = move;
                found = true;
            }
        }

        if (!found)
            break;

        apply_move(best_move, res.sol, used);
        res.obj += best_delta;
    }

    return res;
}

Result steepest_edges_random_candidate(
    const Instance &inst, int start, int candidate_count)
{
    return local_search_candidate(
        inst, start, SearchType::STEEPEST,
        IntraMove::EDGES, false, candidate_count);
}

Result steepest_edges_random_10candidate(
    const Instance &inst, int start)
{
    return steepest_edges_random_candidate(inst, start, 10);
}