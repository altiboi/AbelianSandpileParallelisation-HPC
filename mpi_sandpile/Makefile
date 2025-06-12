# Makefile for Abelian Sandpile simulation with PNG output

# Compiler and flags
CC = mpicc
CFLAGS = -O3 -std=c11 -Wall
INCLUDE = -I/opt/homebrew/include -L/opt/homebrew/lib
LDFLAGS = -lpng

# Target executable
TARGET = sandpile

# Source files
SRC = sandpile_mpi.c

# Build target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f $(TARGET) *.o *.png output*.txt

.PHONY: all clean