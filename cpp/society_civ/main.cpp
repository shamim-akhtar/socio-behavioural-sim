#include "Civilization.h"
#include "Koziel_and_Michalewicz.h"
#include "WeldedBeamDesign.h"
#include <iomanip> // For std::setw
#include <algorithm>
#include <numeric>   // For accumulate
#include <vector>
#include <random>
#include <fstream>

int problem4_1() {
    // 1. Setup Problem Parameters
    const int NUM_RUNS = 10;
    const int MAX_T = 200;

    int m = 100; // Civilization Size
    int n = 2;   // Variables

    const bool USE_RANDOM_SEED = false;

    std::vector<double> lower_bounds = { 13.0, 0.0 };
    std::vector<double> upper_bounds = { 100.0, 100.0 };

    TwoVariableDesign problem4_1;

    // CHANGED: Store the actual Individual objects from each run to analyze later
    // This allows us to print variables/constraints for Average and Worst, not just Best.
    std::vector<Individual> all_run_bests;

    // We still track the global best for the final specific stat check
    Individual global_best_ever(n);
    global_best_ever.objective_value = std::numeric_limits<double>::max();
    int best_run_evaluations = 0;
    bool best_found = false;

    // ============================================================
    // Initialize Data Logger
    // ============================================================
    std::ofstream logFile("simulation_data.csv");
    // Write CSV Header
    logFile << "Run,Time,AgentID,x1,x2,Objective,ClusterID,IsLocalLeader,IsSuperLeader\n";

    std::cout << "Starting Simulation (" << NUM_RUNS << " Runs, " << MAX_T << " Iterations each)...\n";
    std::cout << "Logging data to 'simulation_data.csv'...\n\n";

    // --- RUN LOOP ---
    for (int run = 1; run <= NUM_RUNS; ++run) {
        // Reset evaluations
        problem4_1.reset_evaluations();

        // Seed
        unsigned int current_seed;
        if (USE_RANDOM_SEED) {
            std::random_device rd;
            current_seed = rd();
        }
        else {
            current_seed = run + 10;
        }

        Civilization myCiv(
            m, n, lower_bounds, upper_bounds,
            [&](const Individual& ind) { return problem4_1.get_objective(ind); },
            [&](const Individual& ind) { return problem4_1.get_constraints_violation(ind); },
            current_seed
        );

        myCiv.initialize();
        int t = 0;

        // --- TIME LOOP (Step 9 & 10) ---
        while (t < MAX_T) {
            myCiv.cluster_population();
            myCiv.identify_leaders();
            myCiv.move_society_members();
            myCiv.form_global_society();
            myCiv.identify_super_leaders();
            myCiv.move_global_leaders();

            // Log Data for this Time Step
            myCiv.log_state(logFile, run, t);

            t++;
        }
        myCiv.evaluate_population();

        // --- End of Run Reporting ---
        Individual run_best = myCiv.get_best_solution();
        int evaluations_this_run = problem4_1.evaluations;

        // Store the full individual
        all_run_bests.push_back(run_best);

        std::cout << "Run " << std::setw(2) << run << ": "
            << "Best X = [" << run_best.variables[0] << ", " << run_best.variables[1] << "] "
            << "Obj = " << run_best.objective_value
            << " (Evals: " << evaluations_this_run << ")"
            << std::endl;

        // Track global best
        if (!best_found || run_best.objective_value < global_best_ever.objective_value) {
            global_best_ever = run_best;
            best_run_evaluations = evaluations_this_run;
            best_found = true;
        }
    }

    // --- Final Reporting & Statistics Calculation ---

    // 1. Sort by objective value to find Best and Worst
    std::sort(all_run_bests.begin(), all_run_bests.end(),
        [](const Individual& a, const Individual& b) {
            return a.objective_value < b.objective_value;
        });

    // Best is first (min), Worst is last (max)
    const Individual& best_ind = all_run_bests.front();
    const Individual& worst_ind = all_run_bests.back();

    // 2. Calculate Average Objective
    double sum_obj = 0.0;
    for (const auto& ind : all_run_bests) sum_obj += ind.objective_value;
    double avg_obj = sum_obj / all_run_bests.size();

    // 3. Find the Individual Closest to Average
    auto closest_it = std::min_element(all_run_bests.begin(), all_run_bests.end(),
        [avg_obj](const Individual& a, const Individual& b) {
            return std::abs(a.objective_value - avg_obj) < std::abs(b.objective_value - avg_obj);
        });
    const Individual& avg_ind = *closest_it;

    // Helper Lambda to print details
    auto print_snippet = [&](const std::string& label, const Individual& ind) {
        std::cout << "\n=== " << label << " Result ===\n";

        std::cout << "Variables: [ ";
        for (double v : ind.variables) std::cout << v << " ";
        std::cout << "]\n";

        std::cout << "Objective: " << ind.objective_value << "\n";

        // Print Raw Constraint Values
        std::vector<double> raw_g = problem4_1.get_constraints_raw_values(ind);
        std::cout << "Constraint Values (g(x)): [ ";
        for (double g : raw_g) {
            std::cout << std::fixed << std::setprecision(5) << g << " ";
        }
        std::cout << "]\n";
    };

    std::cout << "\n\n------------------------------------------------------------\n";
    std::cout << "Final Statistical Report (" << NUM_RUNS << " Runs)";
    std::cout << "\n------------------------------------------------------------";
    std::cout << "\nCalculated Average Objective: " << avg_obj << "\n";

    print_snippet("BEST", best_ind);
    print_snippet("AVERAGE (Closest to Mean)", avg_ind);
    print_snippet("WORST", worst_ind);

    return 0;
}


// Welded Beam Design Problem
int problem4_2() {
    // 1. Setup Problem Parameters for Welded Beam Design (Section 4.2)
    const int NUM_RUNS = 10;   // 
    const int MAX_T = 200;     // 

    int m = 100; // Civilization Size 
    int n = 4;   // Variables: h, l, t, b [cite: 586]

    const bool USE_RANDOM_SEED = false;

    // Bounds defined in Eq (25) and text 
    // x1 (h): 0.1 <= x1 <= 2.0
    // x2 (l): 0.1 <= x2 <= 10.0
    // x3 (t): 0.1 <= x3 <= 10.0
    // x4 (b): 0.1 <= x4 <= 2.0
    std::vector<double> lower_bounds = { 0.1, 0.1, 0.1, 0.1 };
    std::vector<double> upper_bounds = { 2.0, 10.0, 10.0, 2.0 };

    WeldedBeamDesign problem4_2;

    // Storage for the best individual from each run for statistical analysis
    std::vector<Individual> all_run_bests;

    // Track global best ever found
    Individual global_best_ever(n);
    global_best_ever.objective_value = std::numeric_limits<double>::max();
    bool best_found = false;

    // ============================================================
    // Initialize Data Logger
    // ============================================================
    // Note: Civilization.h's log_state currently hardcodes printing only x1 and x2.
    // For this 4-variable problem, x3 and x4 won't appear in the CSV unless Civilization.h is updated.
    std::ofstream logFile("simulation_data_welded_beam.csv");

    // Write CSV Header
    logFile << "Run,Time,AgentID,x1,x2,Objective,ClusterID,IsLocalLeader,IsSuperLeader\n";

    std::cout << "Starting Welded Beam Simulation (" << NUM_RUNS << " Runs, " << MAX_T << " Iterations each)...\n";
    std::cout << "Logging data to 'simulation_data_welded_beam.csv'...\n\n";

    // --- RUN LOOP ---
    for (int run = 1; run <= NUM_RUNS; ++run) {
        // Reset evaluations count
        problem4_2.reset_evaluations();

        // Seed generation
        unsigned int current_seed;
        if (USE_RANDOM_SEED) {
            std::random_device rd;
            current_seed = rd();
        }
        else {
            // Predictable seeding for reproducibility
            current_seed = run + 100;
        }

        // Instantiate Civilization with Welded Beam Functors
        Civilization myCiv(
            m, n, lower_bounds, upper_bounds,
            [&](const Individual& ind) { return problem4_2.get_objective(ind); },
            [&](const Individual& ind) { return problem4_2.get_constraints_violation(ind); },
            current_seed
        );

        myCiv.initialize();
        int t = 0;

        // --- TIME LOOP (Steps 9 & 10) ---
        while (t < MAX_T) {
            myCiv.cluster_population();
            myCiv.identify_leaders();
            myCiv.move_society_members();
            myCiv.form_global_society();
            myCiv.identify_super_leaders();
            myCiv.move_global_leaders();

            // Log Data for this Time Step
            myCiv.log_state(logFile, run, t);

            t++;
        }

        // Final evaluation to ensure stats are up to date
        myCiv.evaluate_population();

        // --- End of Run Reporting ---
        Individual run_best = myCiv.get_best_solution();
        int evaluations_this_run = problem4_2.evaluations;

        // Store the full individual
        all_run_bests.push_back(run_best);

        // Print Run Summary
        std::cout << "Run " << std::setw(2) << run << ": "
            << "Obj = " << std::fixed << std::setprecision(5) << run_best.objective_value
            << " | X = [";
        for (size_t i = 0; i < run_best.variables.size(); ++i) {
            std::cout << run_best.variables[i] << (i < run_best.variables.size() - 1 ? ", " : "");
        }
        std::cout << "] (Evals: " << evaluations_this_run << ")\n";

        // Track global best
        if (!best_found || run_best.objective_value < global_best_ever.objective_value) {
            global_best_ever = run_best;
            best_found = true;
        }
    }

    // --- Final Reporting & Statistics Calculation ---

    // 1. Sort by objective value to find Best and Worst
    std::sort(all_run_bests.begin(), all_run_bests.end(),
        [](const Individual& a, const Individual& b) {
            return a.objective_value < b.objective_value;
        });

    // Best is first (min), Worst is last (max)
    const Individual& best_ind = all_run_bests.front();
    const Individual& worst_ind = all_run_bests.back();

    // 2. Calculate Average Objective
    double sum_obj = 0.0;
    for (const auto& ind : all_run_bests) sum_obj += ind.objective_value;
    double avg_obj = sum_obj / all_run_bests.size();

    // 3. Find the Individual Closest to Average
    auto closest_it = std::min_element(all_run_bests.begin(), all_run_bests.end(),
        [avg_obj](const Individual& a, const Individual& b) {
            return std::abs(a.objective_value - avg_obj) < std::abs(b.objective_value - avg_obj);
        });
    const Individual& avg_ind = *closest_it;

    // Helper Lambda to print detailed stats
    auto print_snippet = [&](const std::string& label, const Individual& ind) {
        std::cout << "\n=== " << label << " Result ===\n";

        std::cout << "Variables (h, l, t, b): [ ";
        for (double v : ind.variables) std::cout << std::fixed << std::setprecision(5) << v << " ";
        std::cout << "]\n";

        std::cout << "Objective Cost: " << ind.objective_value << "\n";

        // Print Raw Constraint Values (Positive = Satisfied, Negative = Violated in paper notation)
        // Note: WeldedBeamDesign helper returns values where >= 0 is satisfied.
        std::vector<double> raw_g = problem4_2.get_constraints_raw_values(ind);
        std::cout << "Constraint Margins (>=0 is Feasible): \n[ ";
        for (double g : raw_g) {
            std::cout << g << " ";
        }
        std::cout << "]\n";

        // Check feasibility explicitly
        bool feasible = true;
        for (double g : raw_g) if (g < -1e-6) feasible = false;
        std::cout << "Feasibility Status: " << (feasible ? "FEASIBLE" : "INFEASIBLE") << "\n";
        };

    std::cout << "\n\n------------------------------------------------------------\n";
    std::cout << "Final Statistical Report (" << NUM_RUNS << " Runs)";
    std::cout << "\n------------------------------------------------------------\n";
    std::cout << "Paper Reference Best: 2.4426\n";
    std::cout << "Calculated Average Objective: " << avg_obj << "\n";

    print_snippet("BEST", best_ind);
    print_snippet("AVERAGE (Closest to Mean)", avg_ind);
    print_snippet("WORST", worst_ind);

    return 0;
}

int main() {
    return problem4_2();
}