#include <algorithm>
#include <numeric>
#include <random>
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

    // Exchange two selected nodes.
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

    // Reverse the segment between two edges (2-opt).
    // Assumes symmetric distances.
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

        return inst.dist[a][c] + inst.dist[b][d] - inst.dist[a][b] - inst.dist[c][d];
    }

    // Exchange one selected node with one unselected node.
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

        return inst.dist[prev][candidate] + inst.dist[candidate][next] - inst.dist[prev][curr] - inst.dist[curr][next] + inst.cost[candidate] - inst.cost[curr];
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
            // 2-opt: reverse the segment (i+1 ... j).
            std::reverse(
                sol.begin() + move.i + 1,
                sol.begin() + move.j + 1);
        }
    }

    std::vector<Move> generate_moves(
        int n,
        int k,
        IntraMove intra)
    {
        std::vector<Move> moves;

        // Inter-route moves.
        for (int i = 0; i < k; ++i)
        {
            for (int j = 0; j < n; ++j)
            {
                moves.push_back({MoveType::INTER, i, j});
            }
        }

        // Intra-route moves.
        for (int i = 0; i < k; ++i)
        {
            for (int j = i + 1; j < k; ++j)
            {
                if (intra == IntraMove::NODES)
                {
                    moves.push_back({MoveType::INTRA_NODES, i, j});
                }
                else
                {
                    // Exclude adjacent edges and the cycle boundary pair.
                    if (j == i + 1 || (i == 0 && j == k - 1))
                        continue;

                    moves.push_back({MoveType::INTRA_EDGES, i, j});
                }
            }
        }

        return moves;
    }

} // namespace

Result local_search(
    const Instance &inst,
    int start,
    SearchType search,
    IntraMove intra,
    bool use_greedy_start)
{
    int n = inst.n;
    int k = inst.select_count();

    // Build initial solution.
    Result res;

    if (use_greedy_start)
    {
        // Compare all construction heuristics using the same random start.
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
        // Random starting solution.
        res = random_solution(inst, start);
    }

    // Track selected nodes.
    std::vector<bool> used(n, false);

    for (int v : res.sol)
        used[v] = true;

    // Random generator for greedy neighborhood traversal.
    std::mt19937 rng(42 + start);

    while (true)
    {
        std::vector<Move> moves = generate_moves(n, k, intra);

        // Randomize the complete neighborhood.
        if (search == SearchType::GREEDY)
        {
            std::shuffle(moves.begin(), moves.end(), rng);
        }

        bool found = false;
        int best_delta = 0;
        Move best_move{};

        for (const Move &move : moves)
        {
            int delta;

            if (move.type == MoveType::INTER)
            {
                // Candidate must be currently unselected.
                if (used[move.j])
                    continue;

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

            // Greedy: apply the first improving move.
            if (search == SearchType::GREEDY)
            {
                best_move = move;
                best_delta = delta;
                found = true;
                break;
            }

            // Steepest: remember the best improving move.
            if (!found || delta < best_delta)
            {
                best_delta = delta;
                best_move = move;
                found = true;
            }
        }

        // No improving move: local optimum reached.
        if (!found)
            break;

        apply_move(best_move, res.sol, used);
        res.obj += best_delta;
    }

    return res;
}

Result steepest_nodes_random(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::STEEPEST,
                        IntraMove::NODES, false);
}

Result steepest_nodes_greedy(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::STEEPEST,
                        IntraMove::NODES, true);
}

Result steepest_edges_random(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::STEEPEST,
                        IntraMove::EDGES, false);
}

Result steepest_edges_greedy(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::STEEPEST,
                        IntraMove::EDGES, true);
}

Result greedy_nodes_random(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::GREEDY,
                        IntraMove::NODES, false);
}

Result greedy_nodes_greedy(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::GREEDY,
                        IntraMove::NODES, true);
}

Result greedy_edges_random(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::GREEDY,
                        IntraMove::EDGES, false);
}

Result greedy_edges_greedy(const Instance &inst, int start)
{
    return local_search(inst, start, SearchType::GREEDY,
                        IntraMove::EDGES, true);
}