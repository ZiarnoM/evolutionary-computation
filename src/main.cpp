#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <limits>

#include "core/instance.h"
#include "core/solution.h"
#include "methods/methods.h"

namespace fs = std::filesystem;

static std::vector<Method> methods_for(int assignment)
{
    std::vector<Method> methods = {{"random", random_solution},
                                   {"nn_end", nn_end},
                                   {"nn_any", nn_any},
                                   {"greedy_cycle", greedy_cycle}};
    if (assignment == 1)
        return methods;
    if (assignment == 2)
    {
        methods.push_back({"nn_any_regret", nn_any_regret});
        methods.push_back({"nn_any_weighted", nn_any_weighted});
        methods.push_back({"greedy_cycle_regret", greedy_cycle_regret});
        methods.push_back({"greedy_cycle_weighted", greedy_cycle_weighted});
        return methods;
    }
    if (assignment == 3)
    {
        methods.push_back({"steepest_nodes_random", steepest_nodes_random});
        methods.push_back({"steepest_nodes_greedy", steepest_nodes_greedy});
        methods.push_back({"steepest_edges_random", steepest_edges_random});
        methods.push_back({"steepest_edges_greedy", steepest_edges_greedy});

        methods.push_back({"greedy_nodes_random", greedy_nodes_random});
        methods.push_back({"greedy_nodes_greedy", greedy_nodes_greedy});
        methods.push_back({"greedy_edges_random", greedy_edges_random});
        methods.push_back({"greedy_edges_greedy", greedy_edges_greedy});

        return methods;
    }
    if (assignment == 4)
    {
        methods.push_back({"steepest_edges_10candidate", steepest_edges_random_10candidate});
        return methods;
    }
    if (assignment == 5)
    {
        methods.push_back({"steepest_edges_random", steepest_edges_random});
        methods.push_back({"steepest_edges_10candidate", steepest_edges_random_10candidate});
        methods.push_back({"steepest_edges_lm", steepest_edges_lm});
        return methods;
    }

    return {};
}

struct Stats
{
    Result best;
    int best_start = -1, worst = 0;
    double avg = 0, total_ms = 0, worst_ms = 0, best_ms = std::numeric_limits<double>::infinity();
    std::vector<int> objs;
};

// Runs the method from every node and checks each solution. Exits on an invalid one.
static Stats run_all(const Instance &inst, const Method &m)
{
    Stats st;
    long long sum = 0;
    for (int s = 0; s < inst.n; s++)
    {
        auto t0 = std::chrono::steady_clock::now();
        Result r = m.run(inst, s);
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        st.total_ms += ms;
        st.worst_ms = std::max(st.worst_ms, ms);
        st.best_ms = std::min(st.best_ms, ms);

        std::string err = validate(inst, r.sol);
        if (err.empty() && r.obj != objective(inst, r.sol))
            err = "objective mismatch: method says " + std::to_string(r.obj) +
                  ", recomputed " + std::to_string(objective(inst, r.sol));
        if (!err.empty())
        {
            std::cerr << inst.name << " " << m.name << " start " << s << ": " << err << "\n";
            std::exit(1);
        }

        st.objs.push_back(r.obj);
        if (st.best_start < 0 || r.obj < st.best.obj)
        {
            st.best = r;
            st.best_start = s;
        }
        st.worst = std::max(st.worst, r.obj);
        sum += r.obj;
    }
    st.avg = (double)sum / inst.n;
    return st;
}

int main(int argc, char **argv)
{
    int assignment = 1;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--assignment" && i + 1 < argc)
        {
            assignment = std::stoi(argv[++i]);
        }
        else
        {
            std::cerr << "usage: " << argv[0] << " [--assignment N]\n";
            return 1;
        }
    }

    std::vector<Method> methods = methods_for(assignment);
    if (methods.empty())
    {
        std::cerr << "unknown assignment " << assignment << "\n";
        return 1;
    }

    std::string root = PROJECT_DIR;
    fs::path out = fs::path(root) / "results" / ("assignment" + std::to_string(assignment));
    fs::create_directories(out / "best");

    std::ofstream summary(out / "summary.csv");
    std::ofstream runs(out / "runs.csv");
    summary << std::fixed << std::setprecision(4);
    summary << "instance,method,min,max,avg,best_start,total_ms,min_ms,max_ms,avg_ms\n";
    runs << "instance,method,start,objective\n";

    std::printf("%-5s %-22s %8s %8s %10s %10s\n", "inst", "method", "min", "max", "avg",
                "ms/run");
    for (std::string name : {"TSPA", "TSPB"})
    {
        Instance inst = load_instance(root + "/data/" + name + ".csv");
        for (const Method &m : methods)
        {
            Stats st = run_all(inst, m);
            for (int s = 0; s < inst.n; s++)
                runs << name << "," << m.name << "," << s << "," << st.objs[s] << "\n";
            summary << name << "," << m.name << "," << st.best.obj << "," << st.worst << ","
                    << st.avg << "," << st.best_start << "," << st.total_ms << ","
                    << st.best_ms << "," << st.worst_ms << "," << st.total_ms / inst.n << "\n";
            std::ofstream f(out / "best" / (name + "_" + m.name + ".txt"));
            for (int v : st.best.sol)
                f << v << "\n";

            std::printf("%-5s %-22s %8d %8d %10.1f %10.4f\n", name.c_str(), m.name.c_str(),
                        st.best.obj, st.worst, st.avg, st.total_ms / inst.n);
        }
    }

    // weight of the regret in the weighted criterion, 0 is plain greedy, 1 is pure regret
    if (assignment == 2)
    {
        std::ofstream weights(out / "weights.csv");
        weights << std::fixed << std::setprecision(4);
        weights << "instance,base,w,min,max,avg\n";
        for (std::string name : {"TSPA", "TSPB"})
        {
            Instance inst = load_instance(root + "/data/" + name + ".csv");
            for (bool cycle : {false, true})
            {
                std::string base = cycle ? "greedy_cycle" : "nn_any";
                for (int i = 0; i <= 10; i++)
                {
                    double w = i / 10.0;
                    Method m{base, [cycle, w](const Instance &in, int s)
                             {
                                 return regret_insertion(in, s, cycle, w);
                             }};
                    Stats st = run_all(inst, m);
                    weights << name << "," << base << "," << w << "," << st.best.obj << ","
                            << st.worst << "," << st.avg << "\n";
                }
            }
        }
    }
    if (assignment == 4)
    {
        std::ofstream weights(out / "candidates.csv");
        weights << std::fixed << std::setprecision(4);
        weights << "instance,base,candidate_count,min,max,avg,min_ms,max_ms,avg_ms\n";
        for (std::string name : {"TSPA", "TSPB"})
        {
            Instance inst = load_instance(root + "/data/" + name + ".csv");
            for (int candidate_count : {4, 6, 8, 10, 12, 14, 16, 18, 20})
            {
                Method m{"steepest_edges_random_candidate", [candidate_count](const Instance &in, int s)
                         {
                             return steepest_edges_random_candidate(in, s, candidate_count);
                         }};
                Stats st = run_all(inst, m);
                weights << name << "," << "steepest_edges_random_candidate" << "," << candidate_count
                        << "," << st.best.obj << "," << st.worst << "," << st.avg << "," << st.best_ms << "," << st.worst_ms << "," << st.total_ms / inst.n << "\n";
            }
        }
    }
    std::printf("results written to %s\n", out.c_str());
    return 0;
}
