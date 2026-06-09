#!/bin/bash

# 1. Definimos variables para la prueba
NUM_TRUCKS=10       # El enunciado dice "lanzando 10 camiones"
BASE_ID=100         # Los IDs empezarán en 101, 102... como en el ejemplo
PACKAGES=50         # Cantidad de paquetes fija para la prueba (o aleatoria)

echo "=== Iniciando simulación con $NUM_TRUCKS camiones ==="

# 2. Bucle para lanzar los camiones simultáneamente
for i in $(seq 1 $NUM_TRUCKS)
do
    # Calculamos un ID único para cada camión (101, 102, 103...)
    TRUCK_ID=$((BASE_ID + i))
    
    # Ejecutamos el programa 'truck' pasándole el ID y los paquetes.
    # ¡CRUCIAL!: El '&' al final hace que se ejecute en SEGUNDO PLANO.
    # Esto permite que el bucle continúe y lance el siguiente camión inmediatamente.
    ./truck $TRUCK_ID $PACKAGES &
    
    # Opcional: Un micro-retraso para que no se pisen los printf al arrancar
    sleep 0.05
done

# 3. El comando 'wait' hace que el script de Bash se quede bloqueado aquí
# hasta que ABSOLUTAMENTE TODOS los procesos hijos lanzados en segundo plano (&) terminen.
wait

echo "=== Simulación finalizada. Todos los camiones han procesado su carga ==="
