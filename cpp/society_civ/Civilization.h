#pragma once
#include "Individual.h"

#include <cmath>
#include <limits>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>

class Civilization {
private:
    std::vector<Individual> population;

    // --- NEW: Persistent State for Clustering ---
    // Stores the INDICES of individuals that are hubs (Leaders)
    std::vector<int> hubs;
    // Stores the hub index each individual belongs to (0, 1, 2...)
    std::vector<int> assignments;

    int m_pop_size; // m: Size of civilization [cite: 47]
        int n_variables; // n: Number of design variables [cite: 47]

        // Lower and Upper bounds for each variable (l_j and u_j) 
        std::vector<double> lower_bounds;
    std::vector<double> upper_bounds;

    // Random number generator
    std::mt19937 rng;

public:
    Civilization(int pop_size, int num_vars,
        const std::vector<double>& lb,
        const std::vector<double>& ub)
        : m_pop_size(pop_size), n_variables(num_vars),
        lower_bounds(lb), upper_bounds(ub) {

        // Seed the random number generator
        std::random_device rd;
        rng.seed(rd());
    }

    // Corresponds to Section 3.1: Initialization [cite: 109]
        void initialize() {
        population.clear();
        population.reserve(m_pop_size);

        // R: Random number distribution between 0 and 1 
        std::uniform_real_distribution<double> R(0.0, 1.0);

        for (int i = 0; i < m_pop_size; ++i) {
            Individual ind(n_variables);

            // Apply Eq. (3) for each variable j 
            for (int j = 0; j < n_variables; ++j) {
                double r_val = R(rng);
                // x_j = l_j + R * (u_j - l_j)
                ind.variables[j] = lower_bounds[j] + r_val * (upper_bounds[j] - lower_bounds[j]);
            }

            population.push_back(ind);
        }

        std::cout << "Civilization initialized with " << m_pop_size
            << " individuals." << std::endl;
    }

    // Helper to view the state (for tutorial purposes)
    void print_population_sample(int count = 5) {
        for (int i = 0; i < std::min(count, m_pop_size); ++i) {
            std::cout << "Individual " << i << ": [ ";
            for (double val : population[i].variables) {
                std::cout << val << " ";
            }
            std::cout << "]" << std::endl;
        }
    }

    // Getter for the population
    std::vector<Individual>& get_population() {
        return population;
    }

    // Helper: Calculate Euclidean distance between two individuals
    // Section 3.2: "The distance... is essentially the Euclidean norm" 
    double calculate_distance(const Individual& a, const Individual& b) {
        double sum = 0.0;
        for (int i = 0; i < n_variables; ++i) {
            double diff = a.variables[i] - b.variables[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }

    // Step 2: Cluster the m points into p mutually exclusive clusters
    void cluster_population() {
        if (population.empty()) return;

        // Reset persistent state
        hubs.clear();
        assignments.assign(m_pop_size, -1);

        // --- Step 1: Randomly choose one point as the first hub [cite: 121] ---
            std::uniform_int_distribution<int> dist_idx(0, m_pop_size - 1);
        int first_hub_idx = dist_idx(rng);
        hubs.push_back(first_hub_idx);

        // --- Step 2: Find the point farthest from this hub, make it second hub [cite: 122] ---
            int second_hub_idx = -1;
        double max_dist = -1.0;

        for (int i = 0; i < m_pop_size; ++i) {
            double d = calculate_distance(population[i], population[first_hub_idx]);
            if (d > max_dist) {
                max_dist = d;
                second_hub_idx = i;
            }
        }
        hubs.push_back(second_hub_idx);

        // --- Step 3: Assign points to closest hub [cite: 123-124] ---
        // We do this initially for the first 2 hubs
        for (int i = 0; i < m_pop_size; ++i) {
            double d1 = calculate_distance(population[i], population[hubs[0]]);
            double d2 = calculate_distance(population[i], population[hubs[1]]);
            if (d1 <= d2) assignments[i] = 0; // Belongs to hub 0
            else assignments[i] = 1;          // Belongs to hub 1
        }

        // --- The Clustering Loop (Steps 4 - 7) ---
        while (true) {
            // Step 4: Compute average distance between hubs [cite: 125]
                double total_hub_dist = 0.0;
            int pair_count = 0;
            for (size_t i = 0; i < hubs.size(); ++i) {
                for (size_t j = i + 1; j < hubs.size(); ++j) {
                    total_hub_dist += calculate_distance(population[hubs[i]], population[hubs[j]]);
                    pair_count++;
                }
            }

            double avg_hub_dist = (pair_count > 0) ? (total_hub_dist / pair_count) : 0.0;
            double D = avg_hub_dist / 2.0; // The threshold D [cite: 126]

                // Check distance of every point to its assigned hub
                int farthest_point_idx = -1;
            double max_dist_to_hub = -1.0;

            for (int i = 0; i < m_pop_size; ++i) {
                int hub_idx = hubs[assignments[i]]; // Get the actual individual index of the hub
                double d = calculate_distance(population[i], population[hub_idx]);

                // Track the point farthest from its hub
                if (d > max_dist_to_hub) {
                    max_dist_to_hub = d;
                    farthest_point_idx = i;
                }
            }

            // Termination Condition: If no distance is greater than D, stop [cite: 127]
                if (max_dist_to_hub <= D) {
                    break;
                }

            // Step 5: Make the point farthest from any hub a new hub [cite: 129]
                hubs.push_back(farthest_point_idx);
            int new_hub_internal_idx = hubs.size() - 1;

            // Step 6: Re-assign points if they are closer to the new hub [cite: 130-131]
                for (int i = 0; i < m_pop_size; ++i) {
                    int current_hub_idx = hubs[assignments[i]];
                    double dist_current = calculate_distance(population[i], population[current_hub_idx]);

                    double dist_new = calculate_distance(population[i], population[farthest_point_idx]);

                    if (dist_new < dist_current) {
                        assignments[i] = new_hub_internal_idx;
                    }
                }

            // Step 7: Return to Step 4 (Loop continues) [cite: 132]
        }

        std::cout << "Clustering complete. Created " << hubs.size() << " societies." << std::endl;

        organize_societies();
    }

    // Helper to visualize details (Now uses member variables directly)
    void organize_societies() {
        if (assignments.empty()) return;

        // We can store a vector<vector<int>> representing clusters
        std::vector<std::vector<int>> clusters(hubs.size());
        for (int i = 0; i < m_pop_size; ++i) {
            if (assignments[i] >= 0 && assignments[i] < (int)hubs.size())
                clusters[assignments[i]].push_back(i);
        }

        // Print details for the tutorial
        for (size_t i = 0; i < clusters.size(); ++i) {
            std::cout << "Society " << i << " has " << clusters[i].size() << " individuals." << std::endl;
        }
    }

    // Export to CSV for external plotting
    void export_to_csv(const std::string& filename) {
        if (assignments.empty()) {
            std::cout << "Error: No clusters to export. Run cluster_population() first." << std::endl;
            return;
        }

        std::ofstream file(filename);
        file << "x1,x2,cluster_id\n"; // Header

        for (int i = 0; i < m_pop_size; ++i) {
            // We only save the first 2 variables for 2D plotting
            file << population[i].variables[0] << ","
                << population[i].variables[1] << ","
                << assignments[i] << "\n";
        }
        file.close();
        std::cout << "Data exported to " << filename << std::endl;
    }

    // VISUALIZATION: ASCII Map
    // Maps the first two variables (x1, x2) to a 20x20 grid
    void print_ascii_map() {
        if (assignments.empty() || hubs.empty()) {
            std::cout << "No clusters to display. Run cluster_population() first.\n";
            return;
        }

        const int GRID = 100;
        char grid[GRID][GRID];

        // Fill with empty space
        for (int i = 0; i < GRID; ++i)
            for (int j = 0; j < GRID; ++j)
                grid[i][j] = '.';

        for (int i = 0; i < m_pop_size; ++i) {
            // Normalize x1 and x2 to 0-1 range relative to bounds
            double x = (population[i].variables[0] - lower_bounds[0]) / (upper_bounds[0] - lower_bounds[0]);
            double y = (population[i].variables[1] - lower_bounds[1]) / (upper_bounds[1] - lower_bounds[1]);

            // Convert to grid coordinates
            int c = (int)(x * (GRID - 1));
            int r = (int)(y * (GRID - 1));

            // Flip row index so '0' is at the bottom (Cartesian y-axis)
            r = (GRID - 1) - r;

            if (r >= 0 && r < GRID && c >= 0 && c < GRID) {
                // Check if this individual is a Leader (Hub)
                bool is_hub = false;
                for (int h : hubs) if (h == i) is_hub = true;

                if (is_hub) grid[r][c] = 'X'; // Mark Leader
                else grid[r][c] = '0' + (assignments[i] % 10); // Mark Society ID
            }
        }

        std::cout << "\n   [Map of Civilization (x1 vs x2)]\n";
        std::cout << "   X = Leader, Number = Society ID\n";
        std::cout << "   ------------------------------\n";
        for (int i = 0; i < GRID; ++i) {
            std::cout << "   | ";
            for (int j = 0; j < GRID; ++j) {
                std::cout << grid[i][j] << " ";
            }
            std::cout << "|\n";
        }
        std::cout << "   ------------------------------\n";
    }
};