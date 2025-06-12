# Makefile for OMP Abelian Sandpile simulation

# Compiler and flags
CC = gcc
CFLAGS = -O2 -std=c11 -Wall -fopenmp 

# Target executable
TARGET = sandpile_omp

# Source file
SRC = sandpile_omp.c

# Build target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up build artifacts
clean:
	rm -f $(TARGET) *.o *.png output*.txt

.PHONY: all clean