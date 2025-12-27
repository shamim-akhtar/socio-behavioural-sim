#include "Civilization.h"
#include "Koziel_and_Michalewicz.h" // Your problem definition

int main() {
    // 1. Setup Problem Parameters
    int m = 100; // Civilization Size
    int n = 2;   // 2 Variables (Section 4.1)

    // Bounds from Eq (6) in Section 4.1
    std::vector<double> lower_bounds = { 13.0, 0.0 };
    std::vector<double> upper_bounds = { 100.0, 100.0 };

    // 2. Create the Concrete Problem Logic
    TwoVariableDesign problem4_1;

    // 3. Create Civilization with Functors
    // We bind the problem's methods to the generic std::function arguments
    std::cout << "--- Socio-Behavioural Model: Step 3 (Leader ID) ---\n";

    Civilization myCiv(
        m, n, lower_bounds, upper_bounds,
        // Objective Functor:
        [&](const Individual& ind) { return problem4_1.objective(ind); },
        // Constraint Functor:
        [&](const Individual& ind) { return problem4_1.constraints(ind); }
    );

    // 4. Initialize
    myCiv.initialize();

    // 5. Cluster (Form Societies)
    myCiv.cluster_population();

    // 6. Identify Leaders (Evaluate & Rank)
    myCiv.identify_leaders();
    
    // Step 4 ---
    // Non-leaders learn from leaders and update positions
    myCiv.move_society_members();

    // 7. Visualize Output
    myCiv.print_ascii_map();
    myCiv.export_to_csv("civilization_step4.csv");

    return 0;
}