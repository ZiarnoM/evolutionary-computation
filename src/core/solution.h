#pragma once

#include <string>
#include <vector>

#include "core/instance.h"

// Node indices in cycle order, the edge back to the first node is implied.
using Solution = std::vector<int>;

int cycle_length(const Instance& inst, const Solution& sol);
int nodes_cost(const Instance& inst, const Solution& sol);
int objective(const Instance& inst, const Solution& sol);

// Returns an empty string if the solution is feasible, otherwise what is wrong.
std::string validate(const Instance& inst, const Solution& sol);
