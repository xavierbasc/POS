# Makefile for POS application and product converter utility

# Compiler and flags
CC      = gcc
CFLAGS  = -Os -s
LDFLAGS = -lncurses

# Ejecutables que generaremos
ALL_TARGETS = pos product_converter

# Fuentes del POS
SRC_POS = main.c

# Fuentes del conversor
SRC_CONVERTER = product_converter.c

# Regla principal: construir todo
all: $(ALL_TARGETS)

# Compilar el POS
pos: $(SRC_POS)
	$(CC) $(CFLAGS) -o $@ $(SRC_POS) $(LDFLAGS)

# Compilar el conversor
product_converter: $(SRC_CONVERTER)
	$(CC) $(CFLAGS) -o $@ $(SRC_CONVERTER)

# Build en modo debug
debug: CFLAGS += -DDEBUG
debug: clean $(ALL_TARGETS)

# Limpieza
clean:
	rm -f $(ALL_TARGETS)

.PHONY: all clean debug
