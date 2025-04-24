import matplotlib.pyplot as plt
import pandas as pd
from io import StringIO

# Данные в формате CSV
data = """N=M,T1,T2,S2,T4,S4,T7,S7,T8,S8,T16,S16,T20,S20,T40,S40
40000000,0.422813,1,0.393673,1.07402,0.198368,2.13146,0.104108,4.06128,0.0567781,7.44675,0.0503351,8.39995,0.0327347,12.9163,0.0328157,12.8844,0.0185497,22.7935"""

# Чтение данных
df = pd.read_csv(StringIO(data))

# Извлечение количества потоков и значений ускорения
threads = [1, 2, 4, 7, 8, 16, 20, 40]
# Для потока 1 ускорение всегда 1
# Для остальных потоков берем значения из колонок S2, S4 и т.д.
speedups = [1] + [df[f'S{t}'].iloc[0] for t in threads[1:]]

# Построение графика
plt.figure(figsize=(10, 6))
plt.plot(threads, speedups, 'bo-', label='Измеренное ускорение')
plt.plot(threads, threads, 'r--', label='Линейное ускорение (идеальное)')
plt.xlabel('Количество потоков', fontsize=12)
plt.ylabel('Ускорение', fontsize=12)
plt.title('График ускорения от количества потоков (N=40,000,000)', fontsize=14)
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()
plt.xticks(threads)
plt.savefig('gr.png', format="png", dpi=600)
print(f"График сохранён")
