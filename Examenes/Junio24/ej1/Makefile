# Variables del compilador y opciones
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -pthread

# Nombre del ejecutable final
TARGET = library

# Regla principal (se ejecuta al escribir 'make' a secas)
all: $(TARGET)

# Regla para enlazar el ejecutable final
$(TARGET): library.o
	$(CC) $(CFLAGS) library.o -o $(TARGET) $(LIBS)

# Regla para compilar el código fuente a código objeto
library.o: library.c
	$(CC) $(CFLAGS) -c library.c

# Regla para limpiar los archivos temporales de la carpeta
clean:
	rm -f *.o $(TARGET)

# Evita conflictos si existen archivos que se llamen igual que las reglas
.PHONY: all clean