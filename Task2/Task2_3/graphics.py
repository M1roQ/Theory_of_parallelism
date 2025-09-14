import argparse
import matplotlib.pyplot as plt
import csv

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--data", required=True, help="Путь к CSV-файлу с результатами")
parser.add_argument("-s", "--save", required=True, help="Путь для сохранения графика")
args = parser.parse_args()

with open(args.data, newline='') as csvfile:
    reader = csv.reader(csvfile)
    header = next(reader)
    threads = [int(col[1:]) for col in header if col.startswith('T') and col != 'T1']

    speedups = {}
    variant_names = ["manual", "auto", "static", "dynamic", "guided"]
    idx = 0

    for row in reader:
        spd = []
        for t in threads:
            si = header.index(f"S{t}")
            spd.append(float(row[si]))
        speedups[variant_names[idx]] = spd
        idx += 1

plt.figure(figsize=(8,5))
plt.grid(True)
plt.xlabel("Количество потоков")
plt.ylabel("Ускорение")

X = threads
plt.plot(X, X, 'k--', label="Идеальное")

for name, y in speedups.items():
    plt.plot(X, y, marker='o', label=name)

plt.legend()
plt.tight_layout()
plt.savefig(args.save, dpi=300)
print(f"График ускорения сохранён в {args.save}")
