import matplotlib.pyplot as plt;
import matplotlib.pyplot as plt

cores_data = {}
with open('output.txt') as file:
    for line in file:
        if line.startswith("Core"):
            parts = line.strip().split(":")
            core_id = parts[0].split()[1]
            registers = list(map(int, parts[1].strip().split()))
            cores_data[f"Core {core_id}"] = registers

plt.figure(figsize=(10, 6))
for core, registers in cores_data.items():
    plt.plot(range(32), registers, marker='o', label=core)

plt.title('Register States for Each Core')
plt.xlabel('Register Index (X0 to X31)')
plt.ylabel('Register Value')
plt.legend()
plt.grid(True)
plt.show()
