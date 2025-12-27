import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# ==========================================
# CONFIGURATION
# ==========================================
CSV_FILE = 'simulation_data.csv'
TARGET_RUN = 1   # Which run do you want to animate? (1-10)
FRAME_DELAY = 500 # Milliseconds between frames (lower = faster)
LOWER_BOUND = 0
UPPER_BOUND = 100 # Based on your C++ bounds [0, 100]

# ==========================================
# 1. LOAD DATA
# ==========================================
print(f"Loading {CSV_FILE}...")
df = pd.read_csv(CSV_FILE)

# Filter for the specific run we want to watch
run_data = df[df['Run'] == TARGET_RUN]

# Get unique time steps to animate through
time_steps = sorted(run_data['Time'].unique())
print(f"Found {len(time_steps)} time steps for Run {TARGET_RUN}.")

# ==========================================
# 2. SETUP PLOT
# ==========================================
fig, ax = plt.subplots(figsize=(8, 8))

def update(frame_time):
    ax.clear()
    
    # Extract data for this specific time step
    current_data = run_data[run_data['Time'] == frame_time]
    
    # 1. Plot Regular Citizens (ClusterID determines color)
    # We filter out leaders first to draw them separately/on top if desired, 
    # or just plot everyone and overlay leaders.
    
    # Scatter plot of all agents, colored by ClusterID
    # cmap='tab10' gives distinct colors for up to 10 clusters
    scat = ax.scatter(current_data['x1'], current_data['x2'], 
                      c=current_data['ClusterID'], 
                      cmap='tab20', 
                      s=30, alpha=0.6, label='Citizens')
    
    # 2. Highlight Local Leaders (IsLocalLeader == 1)
    leaders = current_data[current_data['IsLocalLeader'] == 1]
    ax.scatter(leaders['x1'], leaders['x2'], 
               color='blue', marker='D', s=80, edgecolors='white', 
               label='Local Leader')

    # 3. Highlight Super Leaders (IsSuperLeader == 1)
    super_leaders = current_data[current_data['IsSuperLeader'] == 1]
    ax.scatter(super_leaders['x1'], super_leaders['x2'], 
               color='red', marker='*', s=150, edgecolors='black', 
               label='Super Leader')

    # Formatting
    ax.set_xlim(LOWER_BOUND, UPPER_BOUND)
    ax.set_ylim(LOWER_BOUND, UPPER_BOUND)
    ax.set_title(f"Civilization Run {TARGET_RUN} | Time: {frame_time}")
    ax.set_xlabel("Variable x1")
    ax.set_ylabel("Variable x2")
    ax.grid(True, linestyle='--', alpha=0.5)
    
    # Add a legend (only once to avoid duplicate labels)
    # We manually handle legend to ensure all types show up even if count is 0
    ax.legend(loc='upper right')

# ==========================================
# 3. CREATE ANIMATION
# ==========================================
print("Generating animation...")
ani = animation.FuncAnimation(fig, update, frames=time_steps, interval=FRAME_DELAY)

# To Show on Screen:
plt.show()

# To Save as GIF (requires ImageMagick or ffmpeg installed):
# ani.save('civilization_run1.gif', writer='pillow')