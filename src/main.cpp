#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "core/instance.h"
#include "core/solution.h"
#include "methods/methods.h"

namespace fs = std::filesystem;

static std::vector<Method> methods_for(int assignment) {
    if (assignment == 1)
        return {{"random", random_solution},
                {"nn_end", nn_end},
                {"nn_any", nn_any},
                {"greedy_cycle", greedy_cycle}};
    return {};
}

int main(int argc, char** argv) {
    int assignment = 1;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--assignment" && i + 1 < argc) {
            assignment = std::stoi(argv[++i]);
        } else {
            std::cerr << "usage: " << argv[0] << " [--assignment N]\n";
            return 1;
        }
    }

    std::vector<Method> methods = methods_for(assignment);
    if (methods.empty()) {
        std::cerr << "unknown assignment " << assignment << "\n";
        return 1;
    }

    std::string root = PROJECT_DIR;
    fs::path out = fs::path(root) / "results" / ("assignment" + std::to_string(assignment));
    fs::create_directories(out / "best");

    std::ofstream summary(out / "summary.csv");
    std::ofstream runs(out / "runs.csv");
    summary << std::fixed << std::setprecision(4);
    summary << "instance,method,min,max,avg,best_start,total_ms,avg_ms\n";
    runs << "instance,method,start,objective\n";

    std::printf("%-5s %-12s %8s %8s %10s %10s\n", "inst", "method", "min", "max", "avg",
                "ms/run");
    for (std::string name : {"TSPA", "TSPB"}) {
        Instance inst = load_instance(root + "/data/" + name + ".csv");
        for (const Method& m : methods) {
            Result best;
            int best_start = -1, worst = 0;
            long long sum = 0;
            double total_ms = 0;
            for (int s = 0; s < inst.n; s++) {
                auto t0 = std::chrono::steady_clock::now();
                Result r = m.run(inst, s);
                auto t1 = std::chrono::steady_clock::now();
                total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();

                std::string err = validate(inst, r.sol);
                if (err.empty() && r.obj != objective(inst, r.sol))
                    err = "objective mismatch: method says " + std::to_string(r.obj) +
                          ", recomputed " + std::to_string(objective(inst, r.sol));
                if (!err.empty()) {
                    std::cerr << name << " " << m.name << " start " << s << ": " << err << "\n";
                    return 1;
                }

                runs << name << "," << m.name << "," << s << "," << r.obj << "\n";
                if (best_start < 0 || r.obj < best.obj) {
                    best = r;
                    best_start = s;
                }
                worst = std::max(worst, r.obj);
                sum += r.obj;
            }

            double avg = (double)sum / inst.n;
            summary << name << "," << m.name << "," << best.obj << "," << worst << "," << avg
                    << "," << best_start << "," << total_ms << "," << total_ms / inst.n << "\n";
            std::ofstream f(out / "best" / (name + "_" + m.name + ".txt"));
            for (int v : best.sol)
                f << v << "\n";

            std::printf("%-5s %-12s %8d %8d %10.1f %10.4f\n", name.c_str(), m.name.c_str(),
                        best.obj, worst, avg, total_ms / inst.n);
        }
    }
    std::printf("results written to %s\n", out.c_str());
    return 0;
}
