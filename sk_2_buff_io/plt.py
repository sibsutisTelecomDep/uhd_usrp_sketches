import numpy as np
import matplotlib.pyplot as plt

# Загрузка данных из CSV-файла
data = np.loadtxt('/home/dmitry-srs/uhd_usrp_sketches/sk_2_buff_io/build/usrp_samples.csv', delimiter=',', skiprows=1)

# Разделение данных на компоненты I и Q
Q = data[:, 0]
I = data[:, 1]

# Создание массива времени на основе количества выборок
sample_rate = 1e6  # Частота дискретизации в Гц (например, 1 МГц)
time = np.arange(len(I)) / sample_rate

# Построение графиков
plt.figure(figsize=(12, 6))

plt.plot(time, I, label='I', color='blue')
plt.plot(time, Q, label='Q', color='red')
plt.xlabel('Время (с)')
plt.ylabel('Амплитуда I/Q')
plt.title('Компоненты I/Q во времени')
plt.grid(True)

plt.tight_layout()
plt.show()
