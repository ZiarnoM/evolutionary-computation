#include <cstdio>
#include <string>

#include "core/instance.h"
#include "core/solution.h"

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

int main() {
    test_rounding();
    test_checker_examples();
    if (failed) {
        std::printf("%d check(s) failed\n", failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
