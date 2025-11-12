import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

os.makedirs("out", exist_ok=True)

df = pd.read_csv("results.csv")

df_grouped = df.groupby("threads", as_index=False).median()
print("Использованы медианные значения по каждому количеству потоков.")


# === 1. Speedup vs Threads ===
plt.figure(figsize=(10, 6))
plt.plot(df_grouped["threads"], df_grouped["speedup"], "o-", label="Экспериментальный speedup")

max_t = df_grouped["threads"].max()
plt.plot(df_grouped["threads"], df_grouped["threads"], "k--", label="Идеальный speedup (y=x)")

plt.xlabel("Количество потоков")
plt.ylabel("Speedup")
plt.title("Ускорение vs количество потоков (медианные значения)")
plt.grid(True)
plt.legend()
plt.savefig("out/speedup.png", dpi=300, bbox_inches="tight")


# === 2. Efficiency vs Threads ===
plt.figure(figsize=(10, 6))
plt.plot(df_grouped["threads"], df_grouped["efficiency"], "s--", label="Efficiency")
plt.xlabel("Количество потоков")
plt.ylabel("Efficiency")
plt.title("Эффективность vs количество потоков (медианные значения)")
plt.grid(True)
plt.legend()
plt.savefig("out/efficiency.png", dpi=300, bbox_inches="tight")


# === 3. Execution times ===
plt.figure(figsize=(10, 6))
plt.plot(df_grouped["threads"], df_grouped["time_seq"], "o-", label="Время последовательной версии")
plt.plot(df_grouped["threads"], df_grouped["time_par"], "s--", label="Время параллельной версии")
plt.xlabel("Количество потоков")
plt.ylabel("Время (сек)")
plt.title("Время работы vs количество потоков (медианные значения)")
plt.grid(True)
plt.legend()
plt.savefig("out/times.png", dpi=300, bbox_inches="tight")


# === 4. Закон Амдала ===
N = df_grouped["threads"].max()
S_exp = df_grouped.loc[df_grouped["threads"] == N, "speedup"].iloc[0]

P = (1/S_exp - 1) / (1/N - 1)
P = max(0, min(P, 1))

threads_range = np.arange(1, N+1)
speedup_amdahl = 1 / ((1-P) + P / threads_range)

plt.figure(figsize=(10, 6))
plt.plot(threads_range, speedup_amdahl, "r--", label=f"Амдал (p={P:.2%})")
plt.plot(df_grouped["threads"], df_grouped["speedup"], "bo-", label="Экспериментальный speedup (медиана)")
plt.xlabel("Количество потоков")
plt.ylabel("Speedup")
plt.title("Сравнение: закон Амдала и эксперимент (медианные значения)")
plt.grid(True)
plt.legend()
plt.savefig("out/amdahl.png", dpi=300, bbox_inches="tight")

print("Графики сохранены: в директорию ./out/")
