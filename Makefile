# Makefile for Abelian Sandpile simulation with PNG output

# Compiler and flags
CC = gcc
CFLAGS = -O2 -std=c11 -Wall
INCLUDE = -I/opt/homebrew/include -L/opt/homebrew/lib
LDFLAGS = -lpng

# Target executable
TARGET = sandpile_image

# Source files
SRC = sandpile_serial.c

# Build target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f $(TARGET) *.o *.png output*.png

.PHONY: all clean