#include "Civilization.h" // Assuming your class is in this header

int main() {
    // --- 2-Variable Optimization Problem (Section 4.1) ---
    // This allows us to visualize the EXACT search space on the 2D grid.

    int m = 100; // Civilization Size
    int n = 2;   // Reduced to 2 variables 

    // Bounds from Eq (6) in Section 4.1 
    // x1: 13 to 100
    // x2: 0 to 100
    std::vector<double> lower_bounds = { 13.0, 0.0 };
    std::vector<double> upper_bounds = { 100.0, 100.0 };

    std::cout << "--- Socio-Behavioural Model: 2-Variable Test ---\n";

    // 1. Create Civilization
    Civilization myCiv(m, n, lower_bounds, upper_bounds);

    // 2. Initialize Population (Random Scatter)
    myCiv.initialize();

    // 3. Run Clustering Algorithm (Form Societies)
    // The algorithm will group points based on Euclidean distance in 2D space.
    myCiv.cluster_population();

    // 4. Visualize Output
    // Since n=2, this map now shows the REAL distribution of all variables.
    myCiv.print_ascii_map();

    // Export for external tools (Excel/Python)
    myCiv.export_to_csv("civilization_2var.csv");

    return 0;
}