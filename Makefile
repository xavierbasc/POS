# Makefile for POS application and product converter utility

# Compiler
CC      = gcc

# Flags por defecto (compilación "release")
# -Os: optimización para tamaño
# -s:  elimina símbolos para reducir binario
CFLAGS  ?= -Os

# Librerías
LDFLAGS = -lncurses -lform

# Ejecutables que generamos
ALL_TARGETS = pos product_converter

# Fuentes del POS
SRC_POS = main.c

# Fuentes del conversor
SRC_CONVERTER = product_converter.c

# Regla principal: construir todo
.PHONY: all
all: $(ALL_TARGETS)

# Compilar el POS
pos: $(SRC_POS)
	$(CC) $(CFLAGS) -o $@ $(SRC_POS) $(LDFLAGS)

# Compilar el conversor
product_converter: $(SRC_CONVERTER)
	$(CC) $(CFLAGS) -o $@ $(SRC_CONVERTER)

# Build en modo debug:
#  - Se limpian binarios anteriores.
#  - Se vuelve a compilar con -g (símbolos de depuración), -O0 (sin optimización)
#    y -DDEBUG (macro para tu código si quieres usar #ifdef DEBUG, por ejemplo).
.PHONY: debug
debug:
	$(MAKE) clean
	$(MAKE) all CFLAGS="-O0 -g -DDEBUG" LDFLAGS="$(LDFLAGS)"

# Regla para ejecutar el POS directamente
.PHONY: run
run: pos
	./pos

# Limpieza
.PHONY: clean
clean:
	rm -f $(ALL_TARGETS)
