# A Socio-Behavioural Simulation Model for Engineering Design Optimization

This repository contains the C++ implementation of the **Socio-Behavioural Simulation Model**, a constrained optimization algorithm inspired by the social evolution of human civilizations.

This code is based strictly on the research paper:
> **Akhtar, S., Tai, K., & Ray, T. (2002).** *A Socio-Behavioural Simulation Model for Engineering Design Optimization*. Engineering Optimization, 34(4), 341-354.

---

## 1. Introduction
Traditional evolutionary algorithms often rely on "survival of the fittest." In contrast, this model is based on **constructive social interaction**.

The core hypothesis is that individuals in a society improve not just by random mutation, but by observing and learning from **Leaders** (better-performing individuals). As societies improve, they migrate toward better regions of the search space, eventually solving complex engineering problems.

## 2. The Socio-Behavioural Model
The simulation mimics a civilization evolving over time.

### 🏛️ The Civilization & Societies
* **Civilization:** The entire population of $m$ individuals (candidate solutions).
* **Societies (Clusters):** The civilization is divided into mutually exclusive groups called *Societies*. These are formed based on parametric distance (Euclidean distance in the search space), not just fitness.

### 👑 Hierarchy of Leadership
Leadership is established to guide the search:
1. **Society Leaders:** Identified within each cluster using **Pareto Ranking** based on constraint satisfaction.
2. **Super Leaders:** The leaders of all societies form a "Global Society." The best among them become *Super Leaders*.

---

## 3. The Algorithm (Step-by-Step)
The code implements the optimization loop described in **Section 3** of the paper:

### Step 1: Initialization
A civilization of $m$ individuals is created randomly within the variable bounds.

### Step 2: Clustering (Society Formation)
At every time step, individuals are grouped into societies using a **Hub-Center approach**.
* A "hub" is identified, and nearby individuals join that society.
* This ensures diversity and local search capability across the parametric space.

### Step 3: Leader Identification (Pareto Ranking)
Leaders are selected based on strict feasibility rules (Section 3.4):
* **Rank 1:** Feasible individuals (satisfying all constraints).
* **Selection:** If >50% of a society is feasible, leaders are chosen based on the best *Objective Function Value*.

### Step 4: Intra-Society Information Acquisition
Non-leaders improve by moving toward their society's Leader.
* **50% Probability:** Move to a point strictly between the Individual and the Leader.
* **50% Probability:** Explore the outer bounds (away from the leader) to maintain diversity.

### Step 5: Inter-Society Migration (Global Optimization)
The "Global Society" allows information to flow between clusters.
* Society Leaders move toward **Super Leaders**.
* This pulls entire societies out of local optima and toward the global optimum.

---

## 4. Experimental Examples (Included in Code)
This repository includes the implementation of the examples cited in **Section 4** of the paper.

### Example 1: Two-Variable Constrained Optimization
* **Source:** Section 4.1 / Koziel and Michalewicz.
* **Goal:** Minimize a cubic function subject to non-linear inequality constraints.
* **Code Entry:** `problem4_1()` in `main.cpp`.

### Example 2: Welded Beam Design
* **Source:** Section 4.2.
* **Goal:** Minimize the cost of a beam subject to shear stress, bending stress, buckling load, and deflection constraints.
* **Variables:** Thickness ($h$), Length ($l$), Height ($t$), Breadth ($b$).
* **Code Entry:** `problem4_2()` in `main.cpp`.

---

## 📂 Project Structure

| File | Description |
| :--- | :--- |
| **`society_civ/main.cpp`** | The simulation engine. Contains the main loop (Steps 1-5). |
| **`society_civ/Civilization.h`** | Defines the logic for Clustering, Leader selection, and Migration. |
| **`society_civ/Individual.h`** | Defines the agent (variables, constraints, and objective values). |
| **`society_civ/WeldedBeamDesign.h`** | The objective function and constraints for the Welded Beam problem. |
| **`animate_civilization.py`** | A Python script to visualize the societies moving over time. |

---

## 🚀 How to Run

### 1. Compile C++ Simulation
Use any standard C++ compiler (g++, clang, MSVC).
```bash
g++ -o solver society_civ/main.cpp -std=c++11
./solver
```

## 📜 Citation
```bash
@article{akhtar2002socio,
  title={A socio-behavioural simulation model for engineering design optimization},
  author={Akhtar, Shamim and Tai, Kang and Ray, Tapabrata},
  journal={Engineering Optimization},
  volume={34},
  number={4},
  pages={341--354},
  year={2002},
  publisher={Taylor \& Francis}
}
```
