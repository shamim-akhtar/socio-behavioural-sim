#include "Civilization.h"
#include "Koziel_and_Michalewicz.h" 
#include <iomanip> // For std::setw

int main() {
    // 1. Setup Problem Parameters
    const int NUM_RUNS = 10; // "Print the best individual for every run"
    const int MAX_T = 200;   // Maximum iterations (Step 10)

    int m = 100; // Civilization Size
    int n = 2;   // Variables
    
    // Set to 'true' for truly random seeds (different every time you run the program).
    // Set to 'false' for deterministic seeds (reproducible results based on 'run' number).
    const bool USE_RANDOM_SEED = true;

    std::vector<double> lower_bounds = { 13.0, 0.0 };
    std::vector<double> upper_bounds = { 100.0, 100.0 };

    TwoVariableDesign problem4_1;

    // Track Global Best across all runs
    Individual global_best_ever(n);
    global_best_ever.objective_value = std::numeric_limits<double>::max();
    bool best_found = false;

    std::cout << "Starting Simulation (" << NUM_RUNS << " Runs, " << MAX_T << " Iterations each)...\n\n";

    
    // --- RUN LOOP (Perform multiple independent runs) ---
    for (int run = 1; run <= NUM_RUNS; ++run) {
        // --- Determine Seed ---
        unsigned int current_seed;
        if (USE_RANDOM_SEED) {
            // Use hardware/OS entropy for a random seed
            std::random_device rd;
            current_seed = rd();
        }
        else {
            // Deterministic seed based on run number
            current_seed = run + 10;
        }

        // Initialize a new Civilization for this run
        // Note: seed is run + 10 to ensure different random streams
        Civilization myCiv(
            m, n, lower_bounds, upper_bounds,
            [&](const Individual& ind) { return problem4_1.objective(ind); },
            [&](const Individual& ind) { return problem4_1.constraints(ind); },
            current_seed
        );

        myCiv.initialize();
        int t = 0;

        // --- TIME LOOP (Step 9 & 10) ---
        // --- TIME LOOP (Step 9 & 10) ---
        while (t < MAX_T) {
            myCiv.cluster_population();       // Step 2
            myCiv.identify_leaders();         // Step 3
            myCiv.move_society_members();     // Step 4
            myCiv.form_global_society();      // Step 5
            myCiv.identify_super_leaders();   // Step 6
            myCiv.move_global_leaders();      // Step 7 & 8

            // ==============================================================
            // NEW: Print the best individual for current time step t
            // ==============================================================
            // ==============================================================
            // Reporting: Obj, Variables, and Constraints
            // ==============================================================
            Individual current_best = myCiv.get_best_solution();

            std::cout << "  [Run " << run << " | t=" << std::setw(3) << t << "] "
                << "Obj: " << std::fixed << std::setprecision(5) << current_best.objective_value
                << " | X: [" << current_best.variables[0] << ", " << current_best.variables[1] << "]";

            // Print Constraint Violations
            std::cout << " | Violations: [";
            for (size_t k = 0; k < current_best.constraint_violations.size(); ++k) {
                std::cout << current_best.constraint_violations[k];
                // Add comma if not the last element
                if (k < current_best.constraint_violations.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;

            t++; // Step 9
        }

        // --- End of Run Reporting ---
        Individual run_best = myCiv.get_best_solution();

        std::cout << "Run " << std::setw(2) << run << ": "
            << "Best X = [" << run_best.variables[0] << ", " << run_best.variables[1] << "] "
            << "Obj = " << run_best.objective_value
            << std::endl;

        // Check if this run produced the best overall
        if (!best_found || run_best.objective_value < global_best_ever.objective_value) {
            // Simple check: In constrained problems, ensure we prioritize feasible solutions.
            // For simplicity here, we assume valid solutions are found.
            global_best_ever = run_best;
            best_found = true;
        }
    }

    // --- Final Reporting ---
    std::cout << "\n============================================\n";
    std::cout << "FINAL RESULT (Best after " << NUM_RUNS << " runs)\n";
    std::cout << "============================================\n";
    if (best_found) {
        std::cout << "Variables: [ ";
        for (double v : global_best_ever.variables) std::cout << v << " ";
        std::cout << "]\n";

        std::cout << "Objective: " << global_best_ever.objective_value << "\n";

        // NEW: Print Constraint Violations
        std::cout << "Constraints: [ ";
        for (double c : global_best_ever.constraint_violations) {
            std::cout << c << " ";
        }
        std::cout << "]\n";
    }

    return 0;
}