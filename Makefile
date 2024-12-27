# Makefile for POS application using ncurses

# Compiler and flags
CC = gcc
CFLAGS = -Os -s
LDFLAGS = -lncurses

# Target and source files
TARGET = pos
SRC = main.c

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Build in debug mode
debug: CFLAGS += -DDEBUG
debug: clean $(TARGET)

# Clean up
clean:
	rm -f $(TARGET)

.PHONY: all clean debug
