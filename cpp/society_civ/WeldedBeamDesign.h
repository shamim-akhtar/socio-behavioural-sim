#pragma once
#include <vector>
#include <cmath>
#include <functional>
#include <stdexcept>
#include "Individual.h"

// 2. The Concrete Implementation for the "Welded Beam Design" Problem
// Reference: Section 4.2 of the paper 
struct WeldedBeamDesign {

    // Mutable allows modification even in const methods
    mutable int evaluations = 0;

    // Call this at the start of every run
    void reset_evaluations() const {
        evaluations = 0;
    }

    // Constants defined in the problem [cite: 240]
    // P = 6000 lb, L = 14 in, E = 30e6 psi, G = 12e6 psi
    // tau_max = 13600 psi, sigma_max = 30000 psi, delta_max = 0.25 in
    const double P = 6000.0;
    const double L = 14.0;
    const double E = 30.0e6;
    const double G = 12.0e6;
    const double tau_max = 13600.0;
    const double sigma_max = 30000.0;
    const double delta_max = 0.25;

    // Functor for the Objective Function f(x)
    // Minimize Cost f(x) [cite: 220]
    // x1=h, x2=l, x3=t, x4=b
    double get_objective(const Individual& ind) const {
        evaluations++;
        double x1 = ind.variables[0]; // h
        double x2 = ind.variables[1]; // l
        double x3 = ind.variables[2]; // t
        double x4 = ind.variables[3]; // b

        // f(x) = 1.10471*x1^2*x2 + 0.04811*x3*x4*(14.0 + x2)
        return 1.10471 * std::pow(x1, 2) * x2 + 0.04811 * x3 * x4 * (14.0 + x2);
    }

    // Functor for Constraints
    // Returns a vector of VIOLATION values (0.0 if satisfied, positive magnitude if violated)
    // The paper defines constraints as g(x) <= 0.
    // The solver expects g(x) >= 0 (as seen in TwoVariableDesign).
    // Therefore, we transform g_paper <= 0  ==>  g_solver = -g_paper >= 0.
    std::vector<double> get_constraints_violation(const Individual& ind) const {
        const auto raw = get_constraints_raw_values(ind);
        std::vector<double> violations;

        // Logic: The solver treats >= 0 as satisfied.
        // We computed 'val' such that satisfied implies val >= 0.
        // If val < 0, the violation is |val| (or -val).
        for (double val : raw) {
            violations.push_back((val >= 0.0) ? 0.0 : -val);
        }
        return violations;
    }

    // Helper to get raw values where (Value >= 0) implies Satisfaction.
    // Derived from the paper's "g(x) <= 0" form by negating: (Limit - Calculated >= 0)
    std::vector<double> get_constraints_raw_values(const Individual& ind) const {
        if (ind.variables.size() < 4) {
            throw std::runtime_error("WeldedBeamDesign expects 4 variables");
        }

        double x1 = ind.variables[0]; // h
        double x2 = ind.variables[1]; // l
        double x3 = ind.variables[2]; // t
        double x4 = ind.variables[3]; // b

        // --- Intermediate Calculations [cite: 235-239] ---

        // tau' = P / (sqrt(2) * x1 * x2) [cite: 236]
        double tau_prime = P / (std::sqrt(2.0) * x1 * x2);

        // M = P * (L + x2 / 2) [cite: 237]
        double M = P * (L + x2 / 2.0);

        // R = sqrt(x2^2 / 4 + ((x1 + x3) / 2)^2) [cite: 237]
        double R = std::sqrt((std::pow(x2, 2) / 4.0) + std::pow((x1 + x3) / 2.0, 2));

        // J = 2 * { (x1 * x2 / sqrt(2)) * [x2^2 / 12 + ((x1 + x3) / 2)^2] } [cite: 237]
        double J = 2.0 * ((x1 * x2 / std::sqrt(2.0)) * ((std::pow(x2, 2) / 12.0) + std::pow((x1 + x3) / 2.0, 2)));

        // tau'' = M * R / J [cite: 237]
        double tau_double_prime = M * R / J;

        // tau(x) = sqrt( (tau')^2 + (2 * tau' * tau'' * x2) / (2R) + (tau'')^2 ) [cite: 235]
        // Note: The term 2R in denominator cancels with 2 in numerator? 
        // Paper says: (2 * tau' * tau'' * x2) / (2R). 
        double tau = std::sqrt(std::pow(tau_prime, 2) + (2.0 * tau_prime * tau_double_prime * x2) / (2.0 * R) + std::pow(tau_double_prime, 2));

        // sigma(x) = 6PL / (x4 * x3^2) [cite: 237]
        double sigma = (6.0 * P * L) / (x4 * std::pow(x3, 2));

        // delta(x) = 4PL^3 / (E * x4 * x3^3) [cite: 238]
        double delta = (4.0 * P * std::pow(L, 3)) / (E * x4 * std::pow(x3, 3));

        // Pc(x) ... Critical buckling load [cite: 239]
        // Pc = (4.013 * sqrt(E * G * x3^2 * x4^6 / 36) / L^2) * (1 - (x3 / (2L)) * sqrt(E / 4G))
        double term_sqrt = std::sqrt(E * G * std::pow(x3, 2) * std::pow(x4, 6) / 36.0);
        double Pc = (4.013 * term_sqrt / std::pow(L, 2)) * (1.0 - (x3 / (2.0 * L)) * std::sqrt(E / (4.0 * G)));

        // --- Constraints ---
        // Paper form: g(x) <= 0. We convert to Solver form: Result >= 0.

        // 1. Shear stress: tau(x) - tau_max <= 0  ->  tau_max - tau(x) >= 0 [cite: 221]
        double g1 = tau_max - tau;

        // 2. Bending stress: sigma(x) - sigma_max <= 0  ->  sigma_max - sigma(x) >= 0 [cite: 222]
        double g2 = sigma_max - sigma;

        // 3. Geometry: x1 - x4 <= 0  ->  x4 - x1 >= 0 [cite: 223]
        double g3 = x4 - x1;

        // 4. Cost/Budget constraint(?): 0.10471*x1^2 + 0.04811*x3*x4*(14.0 + x2) - 5.0 <= 0 [cite: 224]
        // Note: Paper text differs slightly from objective function coeffs. Using paper constraint text exactly.
        double g4_val = 0.10471 * std::pow(x1, 2) + 0.04811 * x3 * x4 * (14.0 + x2) - 5.0;
        double g4 = -g4_val; // Convert <= 0 to >= 0

        // 5. Geometry: 0.125 - x1 <= 0  ->  x1 - 0.125 >= 0 [cite: 225]
        double g5 = x1 - 0.125;

        // 6. Deflection: delta(x) - delta_max <= 0  ->  delta_max - delta(x) >= 0 [cite: 226]
        double g6 = delta_max - delta;

        // 7. Buckling: P - Pc(x) <= 0  ->  Pc(x) - P >= 0 [cite: 227]
        double g7 = Pc - P;

        return { g1, g2, g3, g4, g5, g6, g7 };
    }
};