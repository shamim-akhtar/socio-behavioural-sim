#pragma once
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>

// Represents a single agent in the civilization
class Individual {
public:
    // x: The design variables (n-dimensional vector) 
    std::vector<double> variables;

    // Stores the violation values for each constraint [cite: 144]
    std::vector<double> constraint_violations;

    // The objective function value f(x) [cite: 136]
    double objective_value;

    // The Pareto rank (1, 2, 3...) based on constraint satisfaction [cite: 159]
    int rank;

    // Constructor to set the size of the design variables
    Individual(int num_variables) {
        variables.resize(num_variables);
        objective_value = 0.0;
        rank = 0;
    }
};