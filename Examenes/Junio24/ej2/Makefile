# Variables del compilador y opciones
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Nombre del ejecutable final
TARGET = space

# Regla principal
all: $(TARGET)

# Regla para enlazar el ejecutable final
$(TARGET): space.o
	$(CC) $(CFLAGS) space.o -o $(TARGET)

# Regla para compilar el código fuente
space.o: space.c
	$(CC) $(CFLAGS) -c space.c

# Regla para limpiar la carpeta
clean:
	rm -f *.o $(TARGET)

.PHONY: all clean