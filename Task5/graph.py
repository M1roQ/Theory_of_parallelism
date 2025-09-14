import argparse
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()

parser.add_argument("-d", "--data", required=True)
parser.add_argument("-i", "--image", required=True)

args = parser.parse_args()
data_path = args.data
image_path = args.image

with open(data_path, "r") as file:
    lines = file.readlines()

# 1 строка — однотпоточное время (например: "Single-threaded: 70.80930")
single_thread_time = float(lines[0].strip().split()[-1])

# 2 строка — список потоков (например: "1 2 3 4 5 6 7 8 9 12")
thread_counts = list(map(int, lines[1].strip().split()))

# 3 строка — времена для каждого количества потоков
times = list(map(float, lines[2].strip().split()))

# 4 строка — ускорения (speedup)
speedups = list(map(float, lines[3].strip().split()))

plt.grid(True)
plt.xlabel("Number of Threads")
plt.ylabel("Speedup")

plt.plot(thread_counts, speedups, "o-", label="Measured Speedup")
plt.plot(thread_counts, thread_counts, "g--", label="Ideal Linear Speedup")

plt.legend()
plt.title(f"Benchmark Speedup (Single-thread time = {single_thread_time:.2f} sec)")

plt.savefig(image_path, format="png", dpi=300)
