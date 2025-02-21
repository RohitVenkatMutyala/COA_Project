import matplotlib.pyplot as plt
import numpy as np

cores_data = {}

# Read data from 'output.txt'
with open('output.txt') as file:
    for line in file:
        if line.startswith("Core"):
            parts = line.strip().split(":")
            core_id = int(parts[0].split()[1])  # Core ID as integer
            registers = list(map(int, parts[1].strip().split()))
            
            # Pad registers to length 32 with zeros if needed
            registers += [0] * (32 - len(registers))
            cores_data[core_id] = registers

# Convert to a 2D numpy array: rows = cores, columns = registers
data_matrix = np.array([cores_data[core] for core in sorted(cores_data)])

# Plotting heatmap
fig, ax = plt.subplots(figsize=(16, 4))
heatmap = ax.imshow(data_matrix, cmap='Blues', aspect='auto')

# Add value labels inside the cells
for i in range(data_matrix.shape[0]):
    for j in range(data_matrix.shape[1]):
        ax.text(j, i, str(data_matrix[i, j]), ha='center', va='center', color='black')

# Axis labels and ticks
ax.set_xticks(np.arange(32))
ax.set_yticks(np.arange(len(cores_data)))
ax.set_xticklabels([f'X{i}' for i in range(32)])
ax.set_yticklabels([f'Core {i}' for i in range(len(cores_data))])

plt.title('Register Values Heatmap for Each Core')
plt.xlabel('Register Index')
plt.ylabel('Cores')

# Colorbar for reference
cbar = plt.colorbar(heatmap)
cbar.set_label('Register Value')

plt.tight_layout()
plt.show()
