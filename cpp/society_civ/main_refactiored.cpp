#include "Civilization.h"
#include "Koziel_and_Michalewicz.h"
#include "WeldedBeamDesign.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// -------------------------------
// Small helpers
// -------------------------------
static double sum_violations(const std::vector<double>& v) {
    double s = 0.0;
    for (double x : v) s += x;
    return s;
}

static std::string format_vec(const std::vector<double>& v, int prec = 6) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        oss << std::fixed << std::setprecision(prec) << v[i];
        if (i + 1 < v.size()) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

// -------------------------------
// Detection idiom for method presence (C++17)
// -------------------------------
template <typename T, typename = void>
struct has_objective : std::false_type {};
template <typename T>
struct has_objective<T, std::void_t<decltype(std::declval<const T&>().objective(std::declval<const Individual&>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct has_constraints : std::false_type {};
template <typename T>
struct has_constraints<T, std::void_t<decltype(std::declval<const T&>().constraints(std::declval<const Individual&>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct has_get_raw_values : std::false_type {};
template <typename T>
struct has_get_raw_values<T, std::void_t<decltype(std::declval<const T&>().get_raw_values(std::declval<const Individual&>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct has_get_objective : std::false_type {};
template <typename T>
struct has_get_objective<T, std::void_t<decltype(std::declval<const T&>().get_objective(std::declval<const Individual&>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct has_get_constraints_violation : std::false_type {};
template <typename T>
struct has_get_constraints_violation<T, std::void_t<decltype(std::declval<const T&>().get_constraints_violation(std::declval<const Individual&>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct has_get_constraints_raw_values : std::false_type {};
template <typename T>
struct has_get_constraints_raw_values<T, std::void_t<decltype(std::declval<const T&>().get_constraints_raw_values(std::declval<const Individual&>()))>>
    : std::true_type {};

// Member field detection: evaluations
template <typename T, typename = void>
struct has_evaluations_field : std::false_type {};
template <typename T>
struct has_evaluations_field<T, std::void_t<decltype(std::declval<const T&>().evaluations)>>
    : std::true_type {};

// -------------------------------
// Unified calls across both naming conventions
// -------------------------------
template <typename ProblemT>
static double call_objective(ProblemT& p, const Individual& ind) {
    if constexpr (has_objective<ProblemT>::value) {
        return p.objective(ind);
    }
    else if constexpr (has_get_objective<ProblemT>::value) {
        return p.get_objective(ind);
    }
    else {
        static_assert(has_objective<ProblemT>::value || has_get_objective<ProblemT>::value,
            "Problem type must provide objective(ind) or get_objective(ind).");
        return 0.0;
    }
}

template <typename ProblemT>
static std::vector<double> call_constraints_violation(ProblemT& p, const Individual& ind) {
    if constexpr (has_constraints<ProblemT>::value) {
        return p.constraints(ind);
    }
    else if constexpr (has_get_constraints_violation<ProblemT>::value) {
        return p.get_constraints_violation(ind);
    }
    else {
        static_assert(has_constraints<ProblemT>::value || has_get_constraints_violation<ProblemT>::value,
            "Problem type must provide constraints(ind) or get_constraints_violation(ind).");
        return {};
    }
}

template <typename ProblemT>
static std::vector<double> call_raw_constraints(ProblemT& p, const Individual& ind) {
    if constexpr (has_get_raw_values<ProblemT>::value) {
        return p.get_raw_values(ind);
    }
    else if constexpr (has_get_constraints_raw_values<ProblemT>::value) {
        return p.get_constraints_raw_values(ind);
    }
    else {
        // Raw constraint reporting is optional; return empty if not available
        return {};
    }
}

template <typename ProblemT>
static long long call_eval_count(const ProblemT& p) {
    if constexpr (has_evaluations_field<ProblemT>::value) {
        return static_cast<long long>(p.evaluations);
    }
    return -1;
}

// -------------------------------
// Common runner for any problem
// -------------------------------
template <typename ProblemT>
static int run_problem(
    const std::string& name,
    ProblemT& problem,
    int n_vars,
    const std::vector<double>& lower_bounds,
    const std::vector<double>& upper_bounds,
    int m_pop_size,
    int max_t,
    int num_runs,
    bool use_random_seed,
    unsigned base_seed
) {
    std::random_device rd;

    std::vector<Individual> all_run_bests;
    all_run_bests.reserve(static_cast<size_t>(num_runs));

    std::vector<unsigned> seeds;
    seeds.reserve(static_cast<size_t>(num_runs));

    std::vector<long long> evals;
    evals.reserve(static_cast<size_t>(num_runs));

    std::cout << "\n============================================================\n";
    std::cout << "Starting " << name << " (" << num_runs << " runs, "
        << max_t << " iterations each)\n";
    std::cout << "m=" << m_pop_size << ", n=" << n_vars << "\n";
    std::cout << "Seed mode: " << (use_random_seed ? "RANDOM" : "DETERMINISTIC")
        << (use_random_seed ? "" : (" (base_seed=" + std::to_string(base_seed) + ")"))
        << "\n";
    std::cout << "============================================================\n";

    // ============================================================
    // Initialize Data Logger
    // ============================================================
    auto safe_filename = [](std::string s) {
        for (char& c : s) {
            if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) c = '_';
        }
        return s;
        };

    const std::string csvFile = safe_filename(name) + ".csv";
    std::ofstream logFile(csvFile);
    // Write CSV Headers
    logFile << "Run,Time,AgentID,x1,x2,Objective,ClusterID,IsLocalLeader,IsSuperLeader\n";

    std::cout << "Starting Simulation (" << num_runs << " Runs, " << max_t << " Iterations each)...\n";
    std::cout << "Logging data to '" << csvFile << "'...\n\n";

    for (int run = 1; run <= num_runs; ++run) {
        // Reset evaluations if supported
        if constexpr (std::is_member_function_pointer_v<decltype(&ProblemT::reset_evaluations)>) {
            problem.reset_evaluations();
        }

        unsigned seed = use_random_seed ? rd() : (base_seed + static_cast<unsigned>(run));
        seeds.push_back(seed);

        Civilization civ(
            m_pop_size, n_vars,
            lower_bounds, upper_bounds,
            [&](const Individual& ind) { return call_objective(problem, ind); },
            [&](const Individual& ind) { return call_constraints_violation(problem, ind); },
            seed
        );

        civ.initialize();

        for (int t = 0; t < max_t; ++t) {
            civ.cluster_population();
            civ.identify_leaders();
            civ.move_society_members();
            civ.form_global_society();
            civ.identify_super_leaders();
            civ.move_global_leaders();


            // Log Data for this Time Step
            civ.log_state(logFile, run, t);
        }

        // IMPORTANT: Ensure final positions are evaluated before selecting best
        civ.evaluate_population();

        Individual run_best = civ.get_best_solution();
        all_run_bests.push_back(run_best);

        long long ev = call_eval_count(problem);
        evals.push_back(ev);

        std::cout << "Run " << std::setw(2) << run
            << " | seed=" << seed
            << " | obj=" << std::fixed << std::setprecision(10) << run_best.objective_value
            << " | sumV=" << std::fixed << std::setprecision(10) << sum_violations(run_best.constraint_violations)
            << " | X=" << format_vec(run_best.variables, 6);

        if (ev >= 0) std::cout << " | evals=" << ev;
        std::cout << "\n";
    }

    // Sort by objective to pick Best/Worst and compute mean objective
    std::sort(all_run_bests.begin(), all_run_bests.end(),
        [](const Individual& a, const Individual& b) {
            return a.objective_value < b.objective_value;
        });

    const Individual& best_ind = all_run_bests.front();
    const Individual& worst_ind = all_run_bests.back();

    double sum_obj = 0.0;
    for (const auto& ind : all_run_bests) sum_obj += ind.objective_value;
    const double avg_obj = sum_obj / static_cast<double>(all_run_bests.size());

    auto closest_it = std::min_element(all_run_bests.begin(), all_run_bests.end(),
        [avg_obj](const Individual& a, const Individual& b) {
            return std::abs(a.objective_value - avg_obj) < std::abs(b.objective_value - avg_obj);
        });
    const Individual& avg_ind = *closest_it;

    auto print_snippet = [&](const std::string& label, const Individual& ind) {
        std::cout << "\n=== " << label << " Result ===\n";
        std::cout << "Variables: " << format_vec(ind.variables, 6) << "\n";
        std::cout << "Objective: " << std::fixed << std::setprecision(10) << ind.objective_value << "\n";
        std::cout << "Violations: " << format_vec(ind.constraint_violations, 10)
            << " (sum=" << std::fixed << std::setprecision(10) << sum_violations(ind.constraint_violations) << ")\n";

        const auto raw = call_raw_constraints(problem, ind);
        if (!raw.empty()) {
            std::cout << "Raw g(x):   " << format_vec(raw, 10) << "\n";
        }
        };

    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "Final Statistical Report (" << name << ", " << num_runs << " runs)\n";
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Calculated Average Objective: " << std::fixed << std::setprecision(10) << avg_obj << "\n";

    print_snippet("BEST", best_ind);
    print_snippet("AVERAGE (Closest to Mean)", avg_ind);
    print_snippet("WORST", worst_ind);

    return 0;
}

// -------------------------------
// Problem entry points
// -------------------------------
static int run_problem4_1() {
    TwoVariableDesign p;

    const int n = 2;
    const int m = 100;
    const int MAX_T = 100;
    const int NUM_RUNS = 20;

    const bool USE_RANDOM_SEED = true;
    const unsigned BASE_SEED = 10;

    std::vector<double> lb = { 13.0, 0.0 };
    std::vector<double> ub = { 100.0, 100.0 };

    return run_problem("problem4_1", p, n, lb, ub, m, MAX_T, NUM_RUNS, USE_RANDOM_SEED, BASE_SEED);
}

static int run_problem4_2() {
    WeldedBeamDesign p;

    const int n = 4;
    const int m = 100;
    const int MAX_T = 100;
    const int NUM_RUNS = 20;

    const bool USE_RANDOM_SEED = true;
    const unsigned BASE_SEED = 100;

    // Keep your chosen bounds; these are common welded-beam bounds
    std::vector<double> lb = { 0.1, 0.1, 0.1, 0.1 };
    std::vector<double> ub = { 2.0, 10.0, 10.0, 2.0 };

    return run_problem("problem4_2", p, n, lb, ub, m, MAX_T, NUM_RUNS, USE_RANDOM_SEED, BASE_SEED);
}

// CLI usage:
//   society_civ.exe            -> problem4_1
//   society_civ.exe 4_1        -> problem4_1
//   society_civ.exe 4_2        -> problem4_2
//   society_civ.exe all        -> both
int main(int argc, char** argv) {
    std::string mode = "4_1";
    if (argc >= 2) mode = argv[1];

    if (mode == "4_1" || mode == "problem4_1") return run_problem4_1();
    if (mode == "4_2" || mode == "problem4_2") return run_problem4_2();
    if (mode == "all") {
        int a = run_problem4_1();
        int b = run_problem4_2();
        return (a != 0 || b != 0) ? 1 : 0;
    }

    std::cerr << "Unknown mode: " << mode << "\n";
    std::cerr << "Usage: " << argv[0] << " [4_1|4_2|all]\n";
    return 1;
}
