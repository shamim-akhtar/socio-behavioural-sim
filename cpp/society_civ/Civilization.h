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
#include <functional> // Required for std::function

class Civilization {
public:
    // Define generic types for our problem functions
    using ObjFunc = std::function<double(const Individual&)>;
    using ConFunc = std::function<std::vector<double>(const Individual&)>;

private:
    std::vector<Individual> population;

    // --- Clustering State ---
    std::vector<int> hubs;        // Geometric centers (used for clustering)
    std::vector<int> assignments; // Society ID for each individual

    // Leadership State
    // Stores the indices of Leaders for each society
    // society_leaders[0] = list of leader indices for Society 0
    std::vector<std::vector<int>> society_leaders;
    
    // Global State (Step 5 & 6)
    std::vector<int> global_society; // Indices of all local leaders collated together
    std::vector<int> super_leaders;  // Indices of the "Best of the Best"

    int m_pop_size;     // m: Size of civilization [cite: 47]
    int n_variables;    // n: Number of design variables [cite: 47]

    std::vector<double> lower_bounds;
    std::vector<double> upper_bounds;
    std::mt19937 rng;

    // --- GENERIC PROBLEM LOGIC ---
    ObjFunc m_objective_fn;
    ConFunc m_constraint_fn;

public:
    // Constructor updated to accept generic functors
    Civilization(int pop_size, int num_vars,
        const std::vector<double>& lb,
        const std::vector<double>& ub,
        ObjFunc obj_func,
        ConFunc con_func,
        unsigned int seed = 10)
        : m_pop_size(pop_size), n_variables(num_vars),
        lower_bounds(lb), upper_bounds(ub),
        m_objective_fn(obj_func), m_constraint_fn(con_func) {

        //std::random_device rd;
        //rng.seed(rd());
        rng.seed(seed);
    }

    // Corresponds to Section 3.1: Initialization [cite: 109]
        void initialize() {
        population.clear();
        population.reserve(m_pop_size);
        std::uniform_real_distribution<double> R(0.0, 1.0);

        for (int i = 0; i < m_pop_size; ++i) {
            Individual ind(n_variables);
            for (int j = 0; j < n_variables; ++j) {
                double r_val = R(rng);
                ind.variables[j] = lower_bounds[j] + r_val * (upper_bounds[j] - lower_bounds[j]);
            }
            population.push_back(ind);
        }
        std::cout << "Civilization initialized with " << m_pop_size << " individuals." << std::endl;
    }

    // --- Helper: Distance ---
    double calculate_distance(const Individual& a, const Individual& b) {
        double sum = 0.0;
        for (int i = 0; i < n_variables; ++i) {
            double diff = a.variables[i] - b.variables[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }

    // --- Step 2: Clustering (Existing logic) ---
    void cluster_population() {
        if (population.empty()) return;

        hubs.clear();
        assignments.assign(m_pop_size, -1);

        // 1. Initial Hubs
        std::uniform_int_distribution<int> dist_idx(0, m_pop_size - 1);
        hubs.push_back(dist_idx(rng));

        int second_hub = -1;
        double max_dist = -1.0;
        for (int i = 0; i < m_pop_size; ++i) {
            double d = calculate_distance(population[i], population[hubs[0]]);
            if (d > max_dist) { max_dist = d; second_hub = i; }
        }
        hubs.push_back(second_hub);

        // Initial assignment
        for (int i = 0; i < m_pop_size; ++i) {
            double d1 = calculate_distance(population[i], population[hubs[0]]);
            double d2 = calculate_distance(population[i], population[hubs[1]]);
            assignments[i] = (d1 <= d2) ? 0 : 1;
        }

        // Clustering Loop
        while (true) {
            double total_dist = 0.0;
            int pairs = 0;
            for (size_t i = 0; i < hubs.size(); ++i) {
                for (size_t j = i + 1; j < hubs.size(); ++j) {
                    total_dist += calculate_distance(population[hubs[i]], population[hubs[j]]);
                    pairs++;
                }
            }
            double D = (pairs > 0) ? (total_dist / pairs) / 2.0 : 0.0;

            int farthest_idx = -1;
            double max_d = -1.0;
            for (int i = 0; i < m_pop_size; ++i) {
                int hub_idx = hubs[assignments[i]];
                double d = calculate_distance(population[i], population[hub_idx]);
                if (d > max_d) { max_d = d; farthest_idx = i; }
            }

            if (max_d <= D) break;

            hubs.push_back(farthest_idx);
            int new_hub_id = hubs.size() - 1;

            for (int i = 0; i < m_pop_size; ++i) {
                int curr = hubs[assignments[i]];
                double d_curr = calculate_distance(population[i], population[curr]);
                double d_new = calculate_distance(population[i], population[farthest_idx]);
                if (d_new < d_curr) assignments[i] = new_hub_id;
            }
        }
        std::cout << "--> Clustering complete. Societies formed: " << hubs.size() << "\n";
        organize_societies();
    }

    // Step 3 - Leader Identification ---

    // 3.1 Evaluate using Generic Functors
    void evaluate_population() {
        for (auto& ind : population) {
            ind.objective_value = m_objective_fn(ind);
            ind.constraint_violations = m_constraint_fn(ind);
        }
    }

    // Helper: Dominance Check for Constraint Satisfaction
    bool dominates(const Individual& a, const Individual& b) {
        bool no_worse = true;
        bool strictly_better = false;
        for (size_t i = 0; i < a.constraint_violations.size(); ++i) {
            if (a.constraint_violations[i] > b.constraint_violations[i]) {
                no_worse = false; break;
            }
            if (a.constraint_violations[i] < b.constraint_violations[i]) strictly_better = true;
        }
        return no_worse && strictly_better;
    }

    // 3.2 Rank Society [cite: 159-160]
    void rank_society(const std::vector<int>& members) {
        std::vector<int> current_pool = members;
        int current_rank = 1;
        while (!current_pool.empty()) {
            std::vector<int> next_pool;
            for (int i : current_pool) {
                bool is_dominated = false;
                for (int j : current_pool) {
                    if (i == j) continue;
                    if (dominates(population[j], population[i])) {
                        is_dominated = true; break;
                    }
                }
                if (!is_dominated) population[i].rank = current_rank;
                else next_pool.push_back(i);
            }
            current_pool = next_pool;
            current_rank++;
        }
    }

    // 3.3 Identify Leaders [cite: 161-169]
    void identify_leaders() {
        evaluate_population();

        int num_societies = hubs.size();
        std::vector<std::vector<int>> societies(num_societies);
        for (int i = 0; i < m_pop_size; ++i)
            if (assignments[i] >= 0) societies[assignments[i]].push_back(i);

        society_leaders.clear();
        society_leaders.resize(num_societies);

        for (int s = 0; s < num_societies; ++s) {
            std::vector<int>& members = societies[s];
            if (members.empty()) continue;

            rank_society(members);

            std::vector<int> rank1;
            double sum_obj = 0.0;
            for (int idx : members) {
                sum_obj += population[idx].objective_value;
                if (population[idx].rank == 1) rank1.push_back(idx);
            }

            double avg_obj = (members.size() > 0) ? sum_obj / members.size() : 0.0;
            bool filter = rank1.size() > (members.size() * 0.5);

            if (!filter) {
                society_leaders[s] = rank1;
            }
            else {
                for (int idx : rank1) {
                    if (population[idx].objective_value <= avg_obj)
                        society_leaders[s].push_back(idx);
                }
                if (society_leaders[s].empty() && !rank1.empty())
                    society_leaders[s].push_back(rank1[0]);
            }
        }
        std::cout << "--> Leaders Identified via Generic Functors.\n";
    }

    // Step 4 Helpers & Logic ---

    // Helper: Check if an individual is currently a leader
    bool is_leader(int index) {
        if (assignments[index] == -1) return false;
        int society_idx = assignments[index];
        if (society_idx >= (int)society_leaders.size()) return false;

        for (int leader_idx : society_leaders[society_idx]) {
            if (leader_idx == index) return true;
        }
        return false;
    }

    // Section 3.5: Information Acquisition Operator [cite: 170-178]
    // Implements the stochastic movement logic
    double acquire_information(double val_ind, double val_leader, double lb, double ub) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double r = dist(rng);

        double min_v = std::min(val_ind, val_leader);
        double max_v = std::max(val_ind, val_leader);

        // Define the 3 regions from Figure 2 [cite: 187]
        if (r < 0.25) {
            // 25% prob: Move between Lower Bound and min(ind, leader) [cite: 173]
            if (min_v <= lb) return lb;
            std::uniform_real_distribution<double> range(lb, min_v);
            return range(rng);
        }
        else if (r < 0.75) {
            // 50% prob: Move between Individual and Leader [cite: 172]
            if (max_v <= min_v) return min_v;
            std::uniform_real_distribution<double> range(min_v, max_v);
            return range(rng);
        }
        else {
            // 25% prob: Move between max(ind, leader) and Upper Bound [cite: 174]
            if (ub <= max_v) return ub;
            std::uniform_real_distribution<double> range(max_v, ub);
            return range(rng);
        }
    }

    // Step 4: Intra-Society Interaction [cite: 97-98]
    void move_society_members() {
        for (int i = 0; i < m_pop_size; ++i) {
            // Leaders do not move in this step [cite: 97]
            if (is_leader(i)) continue;

            int society_id = assignments[i];
            if (society_id == -1 || society_leaders[society_id].empty()) continue;

            // Find nearest leader in the same society [cite: 54]
            int nearest_leader = -1;
            double min_dist = std::numeric_limits<double>::max();

            for (int leader_idx : society_leaders[society_id]) {
                double d = calculate_distance(population[i], population[leader_idx]);
                if (d < min_dist) {
                    min_dist = d;
                    nearest_leader = leader_idx;
                }
            }

            // Apply Information Acquisition Operator for each variable
            if (nearest_leader != -1) {
                for (int j = 0; j < n_variables; ++j) {
                    population[i].variables[j] = acquire_information(
                        population[i].variables[j],
                        population[nearest_leader].variables[j],
                        lower_bounds[j],
                        upper_bounds[j]
                    );
                }
            }
        }
        std::cout << "--> Step 4: Society members moved towards leaders.\n";
    }
    //Step 5 & 6 Helpers ---

    // Step 5: Collate leaders to form global society [cite: 56, 100]
    void form_global_society() {
        global_society.clear();
        for (const auto& leaders : society_leaders) {
            global_society.insert(global_society.end(), leaders.begin(), leaders.end());
        }
        std::cout << "--> Step 5: Global Society formed with " << global_society.size() << " members.\n";
    }

    // Step 6: Identify Super Leaders [cite: 57-59, 102]
    // "The global leaders' society... behaves like any other society."
    void identify_super_leaders() {
        if (global_society.empty()) return;

        super_leaders.clear();

        // 1. Rank the Global Society (Reuse existing ranking logic)
        // Note: This temporarily overwrites 'rank' for these individuals, 
        // which is fine as local movement (Step 4) is already done.
        rank_society(global_society);

        // 2. Filter for Super Leaders (Same logic as Step 3)
        std::vector<int> rank1;
        double sum_obj = 0.0;

        for (int idx : global_society) {
            sum_obj += population[idx].objective_value;
            if (population[idx].rank == 1) rank1.push_back(idx);
        }

        double avg_obj = (global_society.size() > 0) ? sum_obj / global_society.size() : 0.0;

        // Selection Logic
        bool filter = rank1.size() > (global_society.size() * 0.5);

        if (!filter) {
            super_leaders = rank1;
        }
        else {
            for (int idx : rank1) {
                if (population[idx].objective_value <= avg_obj)
                    super_leaders.push_back(idx);
            }
            // Fallback
            if (super_leaders.empty() && !rank1.empty())
                super_leaders.push_back(rank1[0]);
        }

        std::cout << "--> Step 6: Identified " << super_leaders.size() << " Super Leaders.\n";
    }

    // --- AMENDED: Visualization to show 'S' for Super Leaders ---
    void print_ascii_map() {
        if (assignments.empty()) {
            std::cout << "No clusters to display.\n";
            return;
        }

        const int GRID = 100;
        char grid[GRID][GRID];
        for (int i = 0; i < GRID; ++i) for (int j = 0; j < GRID; ++j) grid[i][j] = '.';

        for (int i = 0; i < m_pop_size; ++i) {
            double x = (population[i].variables[0] - lower_bounds[0]) / (upper_bounds[0] - lower_bounds[0]);
            double y = (population[i].variables[1] - lower_bounds[1]) / (upper_bounds[1] - lower_bounds[1]);
            int c = (int)(x * (GRID - 1));
            int r = (int)(y * (GRID - 1));
            r = (GRID - 1) - r;

            if (r >= 0 && r < GRID && c >= 0 && c < GRID) {
                // Check status
                bool is_super = false;
                for (int s : super_leaders) if (s == i) is_super = true;

                bool is_local = false;
                if (!is_super && assignments[i] >= 0) { // Check local leader only if not super
                    for (int l : society_leaders[assignments[i]]) if (l == i) is_local = true;
                }

                if (is_super) grid[r][c] = 'S';      // Super Leader
                else if (is_local) grid[r][c] = 'L'; // Local Leader
                else grid[r][c] = '0' + (assignments[i] % 10);
            }
        }
        std::cout << "\n   [Map: S = Super Leader, L = Local Leader, # = Society ID]\n";
        std::cout << "   ------------------------------\n";
        for (int i = 0; i < GRID; ++i) {
            std::cout << "   | ";
            for (int j = 0; j < GRID; ++j) std::cout << grid[i][j] << " ";
            std::cout << "|\n";
        }
        std::cout << "   ------------------------------\n";
    }

    // --- NEW: Step 7 & 8 Helpers ---

    // Helper: Check if an individual is a Super Leader
    bool is_super_leader(int index) {
        for (int s : super_leaders) {
            if (s == index) return true;
        }
        return false;
    }

    // Step 7: Inter-Society Interaction (Global Leaders move towards Super Leaders) [cite: 104]
    // Step 8: Super leaders do not change their position [cite: 105]
    void move_global_leaders() {
        if (super_leaders.empty()) return;

        for (int leader_idx : global_society) {
            // Step 8: Super leaders do not change position
            if (is_super_leader(leader_idx)) continue;

            // Find closest Super Leader
            int nearest_super = -1;
            double min_dist = std::numeric_limits<double>::max();

            for (int super_idx : super_leaders) {
                double d = calculate_distance(population[leader_idx], population[super_idx]);
                if (d < min_dist) {
                    min_dist = d;
                    nearest_super = super_idx;
                }
            }

            // Apply Information Acquisition Operator
            if (nearest_super != -1) {
                for (int j = 0; j < n_variables; ++j) {
                    population[leader_idx].variables[j] = acquire_information(
                        population[leader_idx].variables[j],
                        population[nearest_super].variables[j],
                        lower_bounds[j],
                        upper_bounds[j]
                    );
                }
            }
        }
        std::cout << "--> Step 7: Global Leaders moved towards Super Leaders.\n";
    }

    // --- AMENDED: Export to CSV to include Super Leader flag ---
    void export_to_csv(const std::string& filename) {
        if (assignments.empty()) return;

        std::ofstream file(filename);
        // Added is_super_leader column
        file << "x1,x2,cluster_id,is_leader,is_super_leader,objective_score\n";

        for (int i = 0; i < m_pop_size; ++i) {
            int is_leader = 0;
            int is_super = 0;

            // Check Local Leader
            if (assignments[i] >= 0 && assignments[i] < (int)society_leaders.size()) {
                for (int l : society_leaders[assignments[i]]) if (l == i) is_leader = 1;
            }
            // Check Super Leader
            for (int s : super_leaders) if (s == i) is_super = 1;

            file << population[i].variables[0] << ","
                << population[i].variables[1] << ","
                << assignments[i] << ","
                << is_leader << ","
                << is_super << ","
                << population[i].objective_value << "\n";
        }
        file.close();
        std::cout << "Data exported to " << filename << std::endl;
    }

    // --- Helpers (PRESERVED) ---

    void organize_societies() {
        if (assignments.empty()) return;
        std::vector<std::vector<int>> clusters(hubs.size());
        for (int i = 0; i < m_pop_size; ++i) {
            if (assignments[i] >= 0 && assignments[i] < (int)hubs.size())
                clusters[assignments[i]].push_back(i);
        }
        for (size_t i = 0; i < clusters.size(); ++i) {
            std::cout << "Society " << i << " has " << clusters[i].size() << " individuals." << std::endl;
        }
    }

    void print_population_sample(int count = 5) {
        for (int i = 0; i < std::min(count, m_pop_size); ++i) {
            std::cout << "Individual " << i << ": [ ";
            for (double val : population[i].variables) std::cout << val << " ";
            std::cout << "]" << std::endl;
        }
    }

    std::vector<Individual>& get_population() { return population; }
};