import csv
import matplotlib.pyplot as plt
import numpy as np
import os

times = []
filename = './timed_data.csv'
with open(filename, 'r') as in_file:
    reader = csv.reader(in_file)
    for row in reader:
        times.append([int(row[0]), int(row[1])])

t_times = np.array(times)
write_times = t_times[:, 0]/1000
read_times = t_times[:, 1]/1000
ave_combined = np.mean(read_times + write_times)
print(f'mean combined {ave_combined}')
#print(read_times)

with plt.xkcd():
    fig, axs = plt.subplots(1, 2, figsize=(12, 4))
    axs[0].hist(read_times, bins=50, log=True)
    axs[0].set_title('read times (ms)')
    axs[0].set_ylabel('counts')
    axs[0].set_xlabel('ms')
    axs[1].hist(write_times, bins=50, log=True)
    axs[1].set_title('write times (ms)')
    axs[1].set_ylabel('counts')
    axs[1].set_xlabel('ms')
    plt.tight_layout()
    plt.savefig('times.png')
    plt.show()
    
