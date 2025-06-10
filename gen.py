# Script to generate a 4096x4096 grid with all elements as 4
grid_size = 1024

# Open file to write
with open('input4.txt', 'w') as f:
    for i in range(grid_size):
        row = ['4'] * grid_size
        f.write(' '.join(row) + '\n')