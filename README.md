Abelian Sandpile – Parallel Implementations

A compact benchmarking suite that implements the Abelian‐sandpile model in three flavours—Serial, OpenMP, and MPI—and provides tooling for automatic grid generation, result collection, post-processing and output verification.

Goal: quantify the benefits and limits of thread-level and process-level parallelism on modern multi-core, multi-socket clusters.

⸻

Repository Layout

input_grids/ ☞ pre-generated starting grids (text format)
MPI/
├─ Makefile ☞ build rules for the MPI version
├─ sandpile_mpi.c ☞ 1-D row decomposition, halo exchange via MPI_Sendrecv
├─ mpi_1_out/ ☞ run outputs for 8–24 ranks
├─ mpi_2_out/ ☞ run outputs for 32–48 ranks
└─ mpi_3_out/ ☞ run outputs for 54–72 ranks
OMP/
├─ Makefile ☞ build rules for the OpenMP version
├─ sandpile_omp.c ☞ parallel for across rows, dynamic scheduling
└─ openmp_results.csv☞ timing results (threads × grid size)
SERIAL/
├─ Makefile ☞ build rules for the baseline serial version
└─ sandpile_serial.c ☞ reference implementation (single core)
serial_out/ ☞ output grids produced by the serial run

compare_outputs.py ☞ CLI tool to diff two output grids
grid_generator.py ☞ utility to create random or custom start grids
HPC_Graphs_weak.ipynb☞ notebook: plots strong/weak scaling & efficiency
README.md ☞ this file

⸻

Quick Start

1. Prerequisites

Software Version Notes
GCC / Clang ≥ 9 -fopenmp for OMP build
OpenMPI / MPICH ≥ 3 tested with OpenMPI 4.1
Python ≥ 3.9 NumPy, Matplotlib, Pandas

Install Python deps:

pip install -r requirements.txt # pandas matplotlib numpy

2. Build all targets

# Serial

cd SERIAL
make

# OpenMP

cd OMP
make

# MPI

cd MPI
make

3. Run examples

# Serial baseline (N = 512²)

./sandpile_serial 512 512 input_grids/input_128.txt img.png

# OpenMP, 24 threads, N = 2048²

./sandpile_omp 2048 2048 input_grids/input_128.txt

# MPI, 8 ranks, N = 2048²

mpirun -np 8 ./sandpile_mpi 2048 2048 input_grids/input_128.txt img.png

⸻

Generate grids (optional):

python grid_generator.py (modify grid size and values)

Compare stable output grids:

python compare_outputs.py serial_out/output_128.txt mpi_1_out/output_128.txt

⸻
