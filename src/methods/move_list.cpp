#include <algorithm>
#include <functional>
#include <vector>

#include "methods/methods.h"

namespace {

// Moves are stored by node ids, not positions, so they stay meaningful after other moves.
// 2-opt (type 0): remove edges a->b and c->d, add a-c and b-d.
// Exchange (type 1): replace v (between a and c) with the unselected node u, here b = v, d = u.
struct Move {
    int delta, type, a, b, c, d;
    bool operator>(const Move& o) const { return delta > o.delta; }
};

struct Tour {
    Solution sol;
    std::vector<int> pos;  // -1 if the node is not selected
    int k;

    int next(int v) const { return sol[(pos[v] + 1) % k]; }
    int prev(int v) const { return sol[(pos[v] + k - 1) % k]; }
    bool edge(int u, int v) const { return pos[u] >= 0 && pos[v] >= 0 && (next(u) == v || prev(u) == v); }

    // reverses positions i..j going forward around the cycle
    void reverse(int i, int j) {
        int len = (j - i + k) % k + 1;
        for (int t = 0; t < len / 2; t++) {
            int x = (i + t) % k, y = (j - t + k) % k;
            std::swap(sol[x], sol[y]);
            pos[sol[x]] = x;
            pos[sol[y]] = y;
        }
    }
};

class MoveList {
    const Instance& inst;
    Tour& t;
    // Moves from a full evaluation are sorted once and read with a cursor, moves added
    // later go to a binary heap. The best move is the better of the two tops.
    std::vector<Move> sorted, heap, later;
    size_t cur = 0;
    bool bulk = false;

    void push(const Move& m) {
        if (bulk) {
            sorted.push_back(m);
            return;
        }
        heap.push_back(m);
        std::push_heap(heap.begin(), heap.end(), std::greater<Move>());
    }

    bool pop(Move& m) {
        bool from_heap = !heap.empty() && (cur == sorted.size() || sorted[cur] > heap.front());
        if (from_heap) {
            std::pop_heap(heap.begin(), heap.end(), std::greater<Move>());
            m = heap.back();
            heap.pop_back();
        } else if (cur < sorted.size()) {
            m = sorted[cur++];
        } else {
            return false;
        }
        return true;
    }

public:
    MoveList(const Instance& inst, Tour& t) : inst(inst), t(t) {}

    void add_2opt(int a, int b, int c, int d) {
        if (a == c || a == d || b == c || b == d)
            return;
        auto& D = inst.dist;
        int delta = D[a][c] + D[b][d] - D[a][b] - D[c][d];
        if (delta >= 0)
            return;
        push({delta, 0, a, b, c, d});
    }

    void add_exchange(int v, int u) {
        int p = t.prev(v), s = t.next(v);
        auto& D = inst.dist;
        int delta = D[p][u] + D[u][s] - D[p][v] - D[v][s] + inst.cost[u] - inst.cost[v];
        if (delta < 0)
            push({delta, 1, p, v, s, u});
    }

    // edge a->b against every other edge, also with the other edge inverted
    void edge_moves(int a, int b) {
        const std::vector<int>&Da = inst.dist[a], &Db = inst.dist[b];
        int ab = Da[b];
        for (int i = 0; i < t.k; i++) {
            int c = t.sol[i], d = t.sol[i + 1 < t.k ? i + 1 : 0];
            if (c == a || c == b || d == a || d == b)
                continue;
            int cd = inst.dist[c][d];
            int d1 = Da[c] + Db[d] - ab - cd, d2 = Da[d] + Db[c] - ab - cd;
            if (d1 < 0)
                push({d1, 0, a, b, c, d});
            if (d2 < 0)
                push({d2, 0, a, b, d, c});
        }
    }

    void node_moves(int v) {
        int p = t.prev(v), s = t.next(v);
        auto& D = inst.dist;
        int base = -D[p][v] - D[v][s] - inst.cost[v];
        for (int u = 0; u < inst.n; u++) {
            if (t.pos[u] >= 0)
                continue;
            int delta = base + D[p][u] + D[u][s] + inst.cost[u];
            if (delta < 0)
                push({delta, 1, p, v, s, u});
        }
    }

    void all_moves() {
        sorted.erase(sorted.begin(), sorted.begin() + cur);
        sorted.insert(sorted.end(), heap.begin(), heap.end());
        heap.clear();
        cur = 0;
        bulk = true;
        for (int i = 0; i < t.k; i++)
            for (int j = i + 2; j < t.k; j++) {
                int a = t.sol[i], b = t.sol[i + 1], c = t.sol[j], d = t.sol[(j + 1) % t.k];
                add_2opt(a, b, c, d);
                add_2opt(a, b, d, c);
            }
        for (int v : t.sol)
            node_moves(v);
        bulk = false;
        std::sort(sorted.begin(), sorted.end(), [](const Move& x, const Move& y) { return x.delta < y.delta; });
    }

    // 0 - remove the move, 1 - apply it, 2 - keep it for later
    int check(const Move& m) const {
        if (m.type == 1) {
            if (t.pos[m.d] >= 0 || !t.edge(m.a, m.b) || !t.edge(m.b, m.c))
                return 0;
            return 1;
        }
        if (!t.edge(m.a, m.b) || !t.edge(m.c, m.d))
            return 0;
        bool fa = t.next(m.a) == m.b, fc = t.next(m.c) == m.d;
        return fa == fc ? 1 : 2;
    }

    void apply(const Move& m) {
        if (m.type == 1) {
            int v = m.b, u = m.d;
            t.sol[t.pos[v]] = u;
            t.pos[u] = t.pos[v];
            t.pos[v] = -1;
            int p = t.prev(u), s = t.next(u);
            edge_moves(p, u);
            edge_moves(u, s);
            for (int x : {p, u, s})
                node_moves(x);
            for (int w : t.sol)
                add_exchange(w, v);
            return;
        }
        int a = m.a, b = m.b, c = m.c, d = m.d;
        if (t.next(a) != b) {
            std::swap(a, b);
            std::swap(c, d);
        }
        // now a->b and c->d, reverse b..c or the other side d..a, whichever is shorter
        int i = t.pos[b], j = t.pos[c];
        if ((j - i + t.k) % t.k < t.k / 2)
            t.reverse(i, j);
        else
            t.reverse(t.pos[d], t.pos[a]);
        if (t.next(a) == c)
            edge_moves(a, c);
        else
            edge_moves(c, a);
        if (t.next(b) == d)
            edge_moves(b, d);
        else
            edge_moves(d, b);
        for (int x : {a, b, c, d})
            node_moves(x);
    }

    // applies the best applicable move from the list, false if there is none
    bool step(int& obj) {
        bool found = false;
        Move m;
        while (pop(m)) {
            int r = check(m);
            if (r == 2) {
                later.push_back(m);
            } else if (r == 1) {
                apply(m);
                obj += m.delta;
                found = true;
                break;
            }
        }
        for (const Move& m : later)
            push(m);
        later.clear();
        return found;
    }
};

}  // namespace

Result steepest_edges_lm(const Instance& inst, int start) {
    Result res = random_solution(inst, start);
    Tour t{res.sol, std::vector<int>(inst.n, -1), (int)res.sol.size()};
    for (int i = 0; i < t.k; i++)
        t.pos[t.sol[i]] = i;

    MoveList lm(inst, t);
    lm.all_moves();
    while (true) {
        if (lm.step(res.obj))
            continue;
        // list has nothing applicable, check the whole neighborhood once more
        lm.all_moves();
        if (!lm.step(res.obj))
            break;
    }
    res.sol = t.sol;
    return res;
}
