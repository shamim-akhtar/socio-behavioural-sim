#pragma once
#include <vector>
#include <cmath>
#include <functional>
#include "Individual.h"

// 1. The Concrete Implementation for the "Two-Variable Problem"
// Reference: Section 4.1 of the paper [cite: 192-209]
struct TwoVariableDesign {

    // Functor for the Objective Function f(x)
    // Minimize f(x) = (x1 - 10)^3 + (x2 - 20)^3 [cite: 195]
    double objective(const Individual& ind) const {
        double x1 = ind.variables[0];
        double x2 = ind.variables[1];
        return std::pow(x1 - 10.0, 3) + std::pow(x2 - 20.0, 3);
    }

    // Functor for Constraints
    // Returns a vector of VIOLATION values (0.0 if satisfied, positive magnitude if violated)
    // Based on Eq (7) logic [cite: 145-148]
    std::vector<double> constraints(const Individual& ind) const {
        double x1 = ind.variables[0];
        double x2 = ind.variables[1];
        std::vector<double> violations;

        // Constraint 1: (x1 - 5)^2 + (x2 - 5)^2 - 100 >= 0 [cite: 197]
        double g1 = std::pow(x1 - 5.0, 2) + std::pow(x2 - 5.0, 2) - 100.0;

        // Constraint 2: -(x1 - 6)^2 - (x2 - 5)^2 + 82.81 >= 0 [cite: 198]
        double g2 = -std::pow(x1 - 6.0, 2) - std::pow(x2 - 5.0, 2) + 82.81;

        // Logic: If g(x) >= 0, violation is 0. Else, violation is |g(x)|
        violations.push_back((g1 >= 0.0) ? 0.0 : std::abs(g1));
        violations.push_back((g2 >= 0.0) ? 0.0 : std::abs(g2));

        return violations;
    }

    // NEW: Helper to get raw g(x) values for reporting
    std::vector<double> get_raw_values(const Individual& ind) const {
        double x1 = ind.variables[0];
        double x2 = ind.variables[1];
        std::vector<double> raw_values;

        // Constraint 1: (x1 - 5)^2 + (x2 - 5)^2 - 100 >= 0
        double g1 = std::pow(x1 - 5.0, 2) + std::pow(x2 - 5.0, 2) - 100.0;

        // Constraint 2: -(x1 - 6)^2 - (x2 - 5)^2 + 82.81 >= 0
        double g2 = -std::pow(x1 - 6.0, 2) - std::pow(x2 - 5.0, 2) + 82.81;

        // Return raw g(x) regardless of sign
        raw_values.push_back(g1);
        raw_values.push_back(g2);

        return raw_values;
    }
};