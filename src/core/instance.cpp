#include "core/instance.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

Instance make_instance(const std::string& name, const std::vector<int>& x,
                       const std::vector<int>& y, const std::vector<int>& cost) {
    Instance inst;
    inst.name = name;
    inst.n = (int)x.size();
    inst.cost = cost;
    inst.dist.assign(inst.n, std::vector<int>(inst.n, 0));
    for (int i = 0; i < inst.n; i++) {
        for (int j = 0; j < inst.n; j++) {
            double dx = x[i] - x[j], dy = y[i] - y[j];
            // same rounding as the solution checker: INT(SQRT(...) + 0.5)
            inst.dist[i][j] = (int)(std::sqrt(dx * dx + dy * dy) + 0.5);
        }
    }
    return inst;
}

Instance load_instance(const std::string& path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open " + path);

    std::vector<int> x, y, cost;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r")
            continue;
        std::stringstream ss(line);
        std::string a, b, c;
        std::getline(ss, a, ';');
        std::getline(ss, b, ';');
        std::getline(ss, c, ';');
        x.push_back(std::stoi(a));
        y.push_back(std::stoi(b));
        cost.push_back(std::stoi(c));
    }

    std::string name = path.substr(path.find_last_of('/') + 1);
    name = name.substr(0, name.find('.'));
    return make_instance(name, x, y, cost);
}
