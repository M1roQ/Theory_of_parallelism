import argparse
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--data", required=True, help="Path to the data file")
parser.add_argument("-s", "--save", required=True, help="Path to save the output image")
args = parser.parse_args()

data_path = args.data
save_path = args.save

data = {}
with open(data_path, "r") as file:
    lines = [line.strip().split(",") for line in file.readlines()]
    
    X = [2, 4, 7, 8, 16, 20, 40]
    
    for line in lines[1:]:
        size = int(line[0]) # Размер матрицы
        data[size] = [[], []]
        # Время (T2, T4, T7, T8, T16, T20, T40) и Ускорение (S2, S4, S7, S8, S16, S20, S40)
        time_values = [float(line[i]) for i in [2, 4, 6, 8, 10, 12, 14]]
        speedup_values = [float(line[i]) for i in [3, 5, 7, 9, 11, 13, 15]]
        data[size][0] = time_values
        data[size][1] = speedup_values

plt.grid(True)
plt.xlabel("Количество потоков")
plt.ylabel("Ускорение / Время")

plt.plot(X, X, "g--", label="Линейное ускорение")

for size in data.keys():
    plt.plot(X, data[size][1], label=f"N={size} (Speedup)")

plt.legend()

plt.savefig(save_path, format="png", dpi=600)
print(f"Saved in {save_path}")
