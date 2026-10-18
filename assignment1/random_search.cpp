#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Instance {
    std::vector<std::vector<long long>> distance;
    std::vector<long long> costs;
};

struct Result {
    int solution_id = 0;
    long long objective = 0;
    long long cycle_length = 0;
    long long selected_node_cost = 0;
    std::vector<int> order; // zero-based node indices
};

static std::vector<std::string> split_csv_line(const std::string& line) {
    // Basic CSV parser supporting quoted fields and escaped double quotes.
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            }
            else {
                in_quotes = !in_quotes;
            }
        }
        else if (ch == ';' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        }
        else {
            field += ch;
        }
    }
    if (in_quotes) {
        throw std::runtime_error("Unterminated quoted CSV field.");
    }
    fields.push_back(field);
    return fields;
}

static long long parse_integer_cost(const std::string& value, int line_number) {
    std::size_t used = 0;
    long long result;
    try {
        result = std::stoll(value, &used);
    }
    catch (...) {
        throw std::runtime_error("Invalid integer cost on line " +
            std::to_string(line_number));
    }
    if (used != value.size()) {
        throw std::runtime_error("Invalid integer cost on line " +
            std::to_string(line_number));
    }
    return result;
}

static Instance read_instance(const std::string& filename, bool has_header) {
    std::ifstream input(filename);
    if (!input) {
        throw std::runtime_error("Cannot open input file: " + filename);
    }

    std::vector<double> xs, ys;
    std::vector<long long> costs;
    std::string line;
    int line_number = 0;

    if (has_header && std::getline(input, line)) {
        ++line_number;
    }

    while (std::getline(input, line)) {
        ++line_number;
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        const auto fields = split_csv_line(line);
        if (fields.size() < 3) {
            throw std::runtime_error("Expected x,y,cost columns on line " +
                std::to_string(line_number));
        }

        try {
            std::size_t used_x = 0, used_y = 0;
            const double x = std::stod(fields[0], &used_x);
            const double y = std::stod(fields[1], &used_y);
            if (used_x != fields[0].size() || used_y != fields[1].size() ||
                !std::isfinite(x) || !std::isfinite(y)) {
                throw std::runtime_error("invalid coordinate");
            }
            xs.push_back(x);
            ys.push_back(y);
            costs.push_back(parse_integer_cost(fields[2], line_number));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Invalid data on line " +
                std::to_string(line_number) + ": " + e.what());
        }
    }

    const int n = static_cast<int>(costs.size());
    if (n == 0) {
        throw std::runtime_error("Input CSV contains no nodes.");
    }

    // Coordinates are used only here. Optimization functions receive no coordinates.
    std::vector<std::vector<long long>> dist(
        n, std::vector<long long>(n, 0));

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double dx = xs[i] - xs[j];
            const double dy = ys[i] - ys[j];
            // Mathematical rounding to nearest integer, ties upward.
            const auto d = static_cast<long long>(
                std::floor(std::hypot(dx, dy) + 0.5));
            dist[i][j] = d;
            dist[j][i] = d;
        }
    }

    return { std::move(dist), std::move(costs) };
}

static Result evaluate_solution(
    int solution_id,
    const std::vector<int>& order,
    const std::vector<std::vector<long long>>& distance,
    const std::vector<long long>& costs) {

    const int k = static_cast<int>(order.size());
    long long cycle_length = 0;
    long long node_cost = 0;

    for (int i = 0; i < k; ++i) {
        const int current = order[i];
        const int next = order[(i + 1) % k];
        cycle_length += distance[current][next];
        node_cost += costs[current];
    }

    return { solution_id, cycle_length + node_cost,
            cycle_length, node_cost, order };
}

static std::vector<Result> random_search(
    const std::vector<std::vector<long long>>& distance,
    const std::vector<long long>& costs,
    int number_of_solutions,
    std::uint32_t seed) {

    if (number_of_solutions <= 0) {
        throw std::invalid_argument("Number of solutions must be positive.");
    }

    const int n = static_cast<int>(costs.size());
    const int k = (n + 1) / 2; // ceil(n/2)
    if (k < 3) {
        throw std::invalid_argument(
            "At least 5 nodes are recommended so the selected set has >= 3 nodes.");
    }

    std::mt19937 rng(seed);
    std::vector<int> nodes(n);
    std::iota(nodes.begin(), nodes.end(), 0);

    std::vector<Result> results;
    results.reserve(number_of_solutions);

    for (int id = 1; id <= number_of_solutions; ++id) {
        // Shuffle all node indices, then take the first k; their order defines the cycle.
        std::shuffle(nodes.begin(), nodes.end(), rng);
        std::vector<int> order(nodes.begin(), nodes.begin() + k);
        results.push_back(evaluate_solution(id, order, distance, costs));
    }

    std::sort(results.begin(), results.end(),
        [](const Result& a, const Result& b) {
            if (a.objective != b.objective)
                return a.objective < b.objective;
            return a.solution_id < b.solution_id;
        });
    return results;
}

static void write_results(const std::vector<Result>& results,
    const std::string& filename) {
    std::ofstream output(filename);
    if (!output) {
        throw std::runtime_error("Cannot write output file: " + filename);
    }

    output << "solution_id,objective,cycle_length,selected_node_cost,"
        "selected_nodes_1_based,cycle_1_based\n";

    for (const auto& r : results) {
        output << r.solution_id << ',' << r.objective << ','
            << r.cycle_length << ',' << r.selected_node_cost << ",\"";

        for (std::size_t i = 0; i < r.order.size(); ++i) {
            if (i) output << ' ';
            output << (r.order[i] + 1);
        }

        output << "\",\"";
        for (std::size_t i = 0; i < r.order.size(); ++i) {
            if (i) output << " -> ";
            output << (r.order[i] + 1);
        }
        output << " -> " << (r.order.front() + 1) << "\"\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            std::cerr
                << "Usage: " << argv[0]
                << " input.csv [--header] [--solutions N] [--seed N]"
                " [--output file.csv]\n";
            return 1;
        }

        std::string input_file = argv[1];
        std::string output_file = "random_search_results.csv";
        bool has_header = false;
        int number_of_solutions = 200;
        std::uint32_t seed = 42;

        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--header") {
                has_header = true;
            }
            else if (arg == "--solutions" && i + 1 < argc) {
                number_of_solutions = std::stoi(argv[++i]);
            }
            else if (arg == "--seed" && i + 1 < argc) {
                seed = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            }
            else if (arg == "--output" && i + 1 < argc) {
                output_file = argv[++i];
            }
            else {
                throw std::invalid_argument("Unknown or incomplete argument: " + arg);
            }
        }

        const Instance instance = read_instance(input_file, has_header);
        const auto results = random_search(
            instance.distance, instance.costs, number_of_solutions, seed);
        write_results(results, output_file);

        const Result& best = results.front();
        std::cout << "Nodes: " << instance.costs.size() << '\n'
            << "Selected per solution: " << (instance.costs.size() + 1) / 2 << '\n'
            << "Random solutions generated: " << results.size() << '\n'
            << "Best objective: " << best.objective << '\n'
            << "  Cycle length: " << best.cycle_length << '\n'
            << "  Selected-node cost: " << best.selected_node_cost << '\n'
            << "  Selected nodes (1-based): ";

        for (std::size_t i = 0; i < best.order.size(); ++i) {
            if (i) std::cout << ' ';
            std::cout << best.order[i] + 1;
        }

        std::cout << "\n  Cycle: ";
        for (int node : best.order) std::cout << node + 1 << " -> ";
        std::cout << best.order.front() + 1 << '\n'
            << "All results saved to: " << output_file << '\n';

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
