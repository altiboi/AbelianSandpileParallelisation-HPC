# Makefile for Abelian Sandpile simulation

# Compilers
MPICC = mpicc
CC = gcc

# Common flags
CFLAGS = -O2 -std=c11 -Wall
PNG_FLAGS = -I/opt/homebrew/include -L/opt/homebrew/lib -lpng

# OpenMP flags for Mac
OMP_FLAGS = -Xpreprocessor -fopenmp
OMP_LIBS = -L/opt/homebrew/Cellar/libomp/20.1.6/lib -lomp -I/opt/homebrew/Cellar/libomp/20.1.6/include

# Target executables
TARGET_SERIAL = sandpile_serial
TARGET_OMP = sandpile_omp
TARGET_MPI = sandpile_mpi

# Source files
SRC_SERIAL = sandpile_serial.c
SRC_OMP = sandpile_omp.c
SRC_MPI = sandpile_mpi.c

# Build all targets
all: $(TARGET_SERIAL) $(TARGET_OMP) $(TARGET_MPI)

# Serial version
$(TARGET_SERIAL): $(SRC_SERIAL)
	$(MPICC) $(CFLAGS) $(PNG_FLAGS) -o $@ $<

# OpenMP version (using gcc with Mac-specific flags)
$(TARGET_OMP): $(SRC_OMP)
	$(CC) $(CFLAGS) $(OMP_FLAGS) $(PNG_FLAGS) $(OMP_LIBS) -o $@ $<

# MPI version
$(TARGET_MPI): $(SRC_MPI)
	$(MPICC) $(CFLAGS) $(PNG_FLAGS) -o $@ $<

# Clean up build artifacts
clean:
	rm -f $(TARGET) *.o *.png output*.txt

.PHONY: all clean
