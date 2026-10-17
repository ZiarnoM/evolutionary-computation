#pragma once

#include <string>
#include <vector>

// Coordinates are only used to build the distance matrix and are not kept,
// so methods can't depend on them.
struct Instance {
    std::string name;
    int n = 0;
    std::vector<std::vector<int>> dist;
    std::vector<int> cost;

    int select_count() const { return (n + 1) / 2; }
};

Instance make_instance(const std::string& name, const std::vector<int>& x,
                       const std::vector<int>& y, const std::vector<int>& cost);
Instance load_instance(const std::string& path);
