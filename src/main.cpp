#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "core/instance.h"
#include "core/solution.h"
#include "methods/methods.h"

static std::vector<Method> methods_for(int assignment) {
    if (assignment == 1)
        return {{"random", random_solution}};
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
    for (std::string name : {"TSPA", "TSPB"}) {
        Instance inst = load_instance(root + "/data/" + name + ".csv");
        for (const Method& m : methods) {
            int best = -1, worst = 0;
            long long sum = 0;
            for (int s = 0; s < inst.n; s++) {
                Result r = m.run(inst, s);
                std::string err = validate(inst, r.sol);
                if (err.empty() && r.obj != objective(inst, r.sol))
                    err = "objective mismatch";
                if (!err.empty()) {
                    std::cerr << name << " " << m.name << " start " << s << ": " << err << "\n";
                    return 1;
                }
                if (best < 0 || r.obj < best)
                    best = r.obj;
                worst = std::max(worst, r.obj);
                sum += r.obj;
            }
            std::printf("%s %-10s min %7d  max %7d  avg %9.1f\n", name.c_str(), m.name.c_str(),
                        best, worst, (double)sum / inst.n);
        }
    }
    return 0;
}
