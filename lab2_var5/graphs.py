import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

df = pd.read_csv("results.csv")


# 1. Speedup vs Threads

plt.figure(figsize=(10, 6))

plt.plot(df["threads"], df["speedup"], "o-", label="Экспериментальный speedup")

max_t = df["threads"].max()
ideal = df["threads"]
plt.plot(ideal, ideal, "k--", label="Идеальный speedup (y=x)")

plt.xlabel("Количество потоков")
plt.ylabel("Speedup")
plt.title("Ускорение vs количество потоков")
plt.grid(True)
plt.legend()
plt.savefig("speedup.png", dpi=300, bbox_inches="tight")


# 2. Efficiency vs Threads

plt.figure(figsize=(10, 6))
plt.plot(df["threads"], df["efficiency"], "s--", label="Efficiency")
plt.xlabel("Количество потоков")
plt.ylabel("Efficiency")
plt.title("Эффективность vs количество потоков")
plt.grid(True)
plt.legend()
plt.savefig("efficiency.png", dpi=300, bbox_inches="tight")


# 3. Execution times

plt.figure(figsize=(10, 6))
plt.plot(df["threads"], df["time_seq"], "o-", label="Время последовательной версии")
plt.plot(df["threads"], df["time_par"], "s--", label="Время параллельной версии")
plt.xlabel("Количество потоков")
plt.ylabel("Время (сек)")
plt.title("Время работы vs количество потоков")
plt.grid(True)
plt.legend()
plt.savefig("times.png", dpi=300, bbox_inches="tight")


# 4. Закон Амдала: теоретический vs фактический speedup

# Используем данные с 12 потоками, чтобы оценить параллельную долю
# Способ: решить уравнение S = 1 / ((1-P) + P/N)

N = df["threads"].max()
S_exp = df.loc[df["threads"] == N, "speedup"].iloc[0]

P = (1/S_exp - 1) / (1/N - 1)
P = max(0, min(P, 1))

threads_range = np.arange(1, N+1)
speedup_amdahl = 1 / ((1-P) + P / threads_range)

plt.figure(figsize=(10, 6))
plt.plot(threads_range, speedup_amdahl, "r--", label=f"Амдал (p={P:.2f})")
plt.plot(df["threads"], df["speedup"], "bo-", label="Экспериментальный speedup")
plt.xlabel("Количество потоков")
plt.ylabel("Speedup")
plt.title("Сравнение: закон Амдала и эксперимент")
plt.grid(True)
plt.legend()
plt.savefig("amdahl.png", dpi=300, bbox_inches="tight")

print("Графики сохранены: speedup.png, efficiency.png, times.png, amdahl.png")
