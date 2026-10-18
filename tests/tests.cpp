#include <algorithm>
#include <cstdio>
#include <string>

#include "core/instance.h"
#include "core/solution.h"
#include "methods/methods.h"

static int failed = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failed++;                                                  \
        }                                                              \
    } while (0)

static void test_rounding() {
    Instance inst = make_instance("t", {0, 3, 1, 1, 2}, {0, 4, 1, 2, 3}, {10, 20, 30, 40, 50});
    CHECK(inst.dist[0][1] == 5);  // 5.0
    CHECK(inst.dist[0][2] == 1);  // 1.414
    CHECK(inst.dist[0][3] == 2);  // 2.236
    CHECK(inst.dist[0][4] == 4);  // 3.606
    CHECK(inst.dist[1][0] == inst.dist[0][1]);
    CHECK(inst.dist[2][2] == 0);
    CHECK(inst.select_count() == 3);

    Solution s = {0, 1, 4};
    CHECK(cycle_length(inst, s) == 5 + inst.dist[1][4] + 4);
    CHECK(nodes_cost(inst, s) == 80);
    CHECK(validate(inst, s).empty());
    CHECK(!validate(inst, {0, 1}).empty());
    CHECK(!validate(inst, {0, 1, 1}).empty());
    CHECK(!validate(inst, {0, 1, 5}).empty());
}

// Solutions typed into column F of "Solution checker.xlsx" and the totals it shows.
static void test_checker_examples() {
    Instance a = load_instance(std::string(DATA_DIR) + "/TSPA.csv");
    Instance b = load_instance(std::string(DATA_DIR) + "/TSPB.csv");
    CHECK(a.n == 200 && b.n == 200);
    CHECK(a.cost[0] == 1607);

    Solution sa = {140, 108, 69, 18, 22, 193, 41, 139, 42, 160, 177, 10, 190, 4, 112, 184, 43,
                   116, 65, 123, 127, 162, 151, 133, 135, 154, 180, 53, 100, 26, 86, 75, 120,
                   44, 25, 16, 171, 175, 113, 56, 31, 78, 145, 196, 81, 90, 165, 40, 185, 106,
                   178, 49, 14, 144, 62, 9, 148, 167, 52, 55, 57, 92, 129, 82, 2, 101, 1, 97,
                   152, 124, 94, 63, 80, 176, 137, 51, 118, 59, 115, 46, 0, 89, 183, 143, 117,
                   93, 101, 99, 135, 51, 112, 66, 6, 156, 98, 190, 72, 94, 12, 73};
    Solution sb = {90, 51, 121, 131, 122, 133, 107, 40, 63, 135, 38, 27, 16, 1, 156, 198, 117,
                   193, 31, 54, 73, 136, 190, 80, 45, 175, 78, 5, 177, 36, 61, 91, 141, 77, 81,
                   153, 187, 163, 89, 127, 137, 114, 103, 113, 176, 194, 166, 86, 185, 95, 130,
                   99, 179, 66, 94, 47, 148, 60, 20, 28, 149, 4, 140, 183, 152, 170, 34, 55, 18,
                   62, 124, 106, 143, 35, 109, 0, 29, 160, 33, 144, 111, 82, 21, 8, 104, 138, 11,
                   139, 168, 195, 13, 145, 15, 3, 70, 132, 169, 188, 6, 147};

    CHECK(nodes_cost(a, sa) + cycle_length(a, sa) == 253566);
    CHECK(nodes_cost(a, sa) == 95466);
    CHECK(cycle_length(a, sa) == 158100);
    CHECK(objective(b, sb) == 200088);
    CHECK(nodes_cost(b, sb) == 50687);

    // the TSPA example in the checker repeats some nodes
    CHECK(validate(a, sa).rfind("duplicate", 0) == 0);
    CHECK(validate(b, sb).empty());
}

static void test_random() {
    Instance a = load_instance(std::string(DATA_DIR) + "/TSPA.csv");
    Result r1 = random_solution(a, 0);
    Result r2 = random_solution(a, 0);
    Result r3 = random_solution(a, 1);
    CHECK(validate(a, r1.sol).empty());
    CHECK(r1.obj == objective(a, r1.sol));
    CHECK(r1.sol == r2.sol);
    CHECK(r1.sol != r3.sol);
}

// Nodes on a line: 0 at x=0, 1 at x=10, 2 at x=5 with cost 6, the rest far away.
// From node 0 the nearest node by distance is 2, but with its cost node 1 is better.
// NN-any should then put node 2 between 1 and 0, where it adds no length.
static void test_greedy_small() {
    Instance t = make_instance("t", {0, 10, 5, 1000, 2000, 3000}, {0, 0, 0, 0, 0, 0},
                               {0, 0, 6, 0, 0, 0});

    Result r = nn_end(t, 0);
    CHECK((r.sol == Solution{0, 1, 2}));
    CHECK(r.obj == 26);

    r = nn_any(t, 0);
    CHECK((r.sol == Solution{1, 2, 0}));
    CHECK(r.obj == 26);

    // second node: 2*5+6 = 16 < 2*10 = 20, then node 1 goes into the first edge
    r = greedy_cycle(t, 0);
    CHECK((r.sol == Solution{0, 1, 2}));
    CHECK(r.obj == 26);

    // costs only: node 1 (cost 10) loses to node 2 although it is closer
    Instance c = make_instance("c", {0, 1, 3, 100}, {0, 0, 0, 0}, {0, 10, 0, 0});
    CHECK((nn_end(c, 0).sol == Solution{0, 2}));
    CHECK((greedy_cycle(c, 0).sol == Solution{0, 2}));
}

static void test_all_starts() {
    std::vector<Method> methods = {{"random", random_solution},
                                   {"nn_end", nn_end},
                                   {"nn_any", nn_any},
                                   {"greedy_cycle", greedy_cycle}};
    for (std::string name : {"TSPA", "TSPB"}) {
        Instance inst = load_instance(std::string(DATA_DIR) + "/" + name + ".csv");
        for (const Method& m : methods) {
            int bad = 0;
            for (int s = 0; s < inst.n; s++) {
                Result r = m.run(inst, s);
                bool ok = validate(inst, r.sol).empty() && r.obj == objective(inst, r.sol);
                if (m.name != "random")
                    ok = ok && std::find(r.sol.begin(), r.sol.end(), s) != r.sol.end();
                if (!ok)
                    bad++;
            }
            if (bad)
                std::printf("%s %s: %d bad solutions\n", name.c_str(), m.name.c_str(), bad);
            CHECK(bad == 0);
        }
    }
}

int main() {
    test_rounding();
    test_checker_examples();
    test_random();
    test_greedy_small();
    test_all_starts();
    if (failed) {
        std::printf("%d check(s) failed\n", failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
