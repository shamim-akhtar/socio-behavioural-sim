#pragma once
#include <vector>
#include <cmath>
#include <functional>
#include <stdexcept>
#include "Individual.h"

// 2. The Concrete Implementation for the "Welded Beam Design" Problem
// Reference: Section 4.2 of the paper
struct WeldedBeamDesign {

    mutable int evaluations = 0;

    void reset_evaluations() const {
        evaluations = 0;
    }

    // Constants
    const double P = 6000.0;
    const double L = 14.0;
    const double E = 30.0e6;
    const double G = 12.0e6;
    const double tau_max = 13600.0;
    const double sigma_max = 30000.0;
    const double delta_max = 0.25;

    double get_objective(const Individual& ind) const {
        evaluations++;
        double x1 = ind.variables[0]; // h
        double x2 = ind.variables[1]; // l
        double x3 = ind.variables[2]; // t
        double x4 = ind.variables[3]; // b

        return 1.10471 * std::pow(x1, 2) * x2 + 0.04811 * x3 * x4 * (14.0 + x2);
    }

    // Returns VIOLATION magnitude (Must be >= 0)
    std::vector<double> get_constraints_violation(const Individual& ind) const {
        const auto raw = get_constraints_raw_values(ind);
        std::vector<double> violations;

        // In this problem, g(x) <= 0 is satisfied.
        // If g(x) > 0, it is a violation.
        for (double val : raw) {
            violations.push_back((val <= 0.0) ? 0.0 : val);
        }
        return violations;
    }

    // Returns raw g(x) values exactly as defined in Paper Eq (11)-(17).
    // Feasible <= 0.
    std::vector<double> get_constraints_raw_values(const Individual& ind) const {
        if (ind.variables.size() < 4) {
            throw std::runtime_error("WeldedBeamDesign expects 4 variables");
        }

        double x1 = ind.variables[0]; // h
        double x2 = ind.variables[1]; // l
        double x3 = ind.variables[2]; // t
        double x4 = ind.variables[3]; // b

        // --- Intermediate Calculations ---
        double tau_prime = P / (std::sqrt(2.0) * x1 * x2);
        double M = P * (L + x2 / 2.0);
        double R = std::sqrt((std::pow(x2, 2) / 4.0) + std::pow((x1 + x3) / 2.0, 2));
        double J = 2.0 * ((x1 * x2 / std::sqrt(2.0)) * ((std::pow(x2, 2) / 12.0) + std::pow((x1 + x3) / 2.0, 2)));
        double tau_double_prime = M * R / J;
        double tau = std::sqrt(std::pow(tau_prime, 2) + (2.0 * tau_prime * tau_double_prime * x2) / (2.0 * R) + std::pow(tau_double_prime, 2));
        double sigma = (6.0 * P * L) / (x4 * std::pow(x3, 2));
        double delta = (4.0 * P * std::pow(L, 3)) / (E * x4 * std::pow(x3, 3));
        double term_sqrt = std::sqrt(E * G * std::pow(x3, 2) * std::pow(x4, 6) / 36.0);
        double Pc = (4.013 * term_sqrt / std::pow(L, 2)) * (1.0 - (x3 / (2.0 * L)) * std::sqrt(E / (4.0 * G)));

        // --- Constraints (g(x) <= 0) ---

        // 1. Shear stress: tau(x) - tau_max <= 0
        double g1 = tau - tau_max;

        // 2. Bending stress: sigma(x) - sigma_max <= 0
        double g2 = sigma - sigma_max;

        // 3. Geometry: x1 - x4 <= 0
        double g3 = x1 - x4;

        // 4. Cost Constraint: 0.10471*x1^2 + ... - 5.0 <= 0
        double g4 = 0.10471 * std::pow(x1, 2) + 0.04811 * x3 * x4 * (14.0 + x2) - 5.0;

        // 5. Geometry: 0.125 - x1 <= 0
        double g5 = 0.125 - x1;

        // 6. Deflection: delta(x) - delta_max <= 0
        double g6 = delta - delta_max;

        // 7. Buckling: P - Pc(x) <= 0
        double g7 = P - Pc;

        return { g1, g2, g3, g4, g5, g6, g7 };
    }
};