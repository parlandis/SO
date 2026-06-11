#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_AFORO 3  // Aforo predefinido por el apartado A

// Estructura para pasar la información a cada usuario (hilo)
typedef struct {
    int id;
    int es_profesor; // 1 si es profesor, 0 si es estudiante
} usuario_t;

// Variables de estado del Monitor de Sincronización
int ocupantes = 0;          // Personas actualmente dentro de la biblioteca
int profes_esperando = 0;   // Número de profesores en la cola de espera
int biblioteca_abierta = 1; // 1 = Abierta, 0 = Cerrada por vacaciones

// Recursos de sincronización POSIX
pthread_mutex_t mutex;
pthread_cond_t cond_estudiantes;
pthread_cond_t cond_profesores;
pthread_cond_t cond_vacaciones;

/*
 * entrar_biblioteca
 * - Implementa el protocolo de entrada al monitor respetando las prioridades.
 */
void entrar_biblioteca(int id, int es_profesor) {
    pthread_mutex_lock(&mutex);

    if (es_profesor) {
        profes_esperando++;
        printf("User %d (professor) waiting on the queue.\n", id);
        
        // Un profesor espera si: aforo completo O la biblioteca está cerrada por vacaciones
        while (ocupantes >= MAX_AFORO || !biblioteca_abierta) {
            pthread_cond_wait(&cond_profesores, &mutex);
        }
        profes_esperando--;
    } 
    else {
        printf("User %d (student) waiting on the queue.\n", id);
        
        // Un estudiante espera si: aforo completo O la biblioteca está cerrada 
        // O hay profesores en la cola (Prioridad estricta de la Regla 2)
        while (ocupantes >= MAX_AFORO || !biblioteca_abierta || profes_esperando > 0) {
            pthread_cond_wait(&cond_estudiantes, &mutex);
        }
    }

    // El usuario consigue entrar
    ocupantes++;
    printf("User %d (%s) is reading books for 2 seconds.\n", id, es_profesor ? "professor" : "student");
    
    pthread_mutex_unlock(&mutex);
}

/*
 * salir_biblioteca
 * - Implementa el protocolo de salida del monitor y despierta a los hilos bloqueados.
 */
void salir_biblioteca(int id, int es_profesor) {
    pthread_mutex_lock(&mutex);

    ocupantes--;
    printf("User %d (%s) leaves the library.\n", id, es_profesor ? "professor" : "student");

    // [Regla 4] Si la biblioteca está cerrada por vacaciones y se queda vacía,
    // avisamos al hilo de control de vacaciones si estuviera esperando
    if (!biblioteca_abierta && ocupantes == 0) {
        pthread_cond_signal(&cond_vacaciones);
    }

    // Al salir, despertamos primero a los profesores si hay alguno esperando (Regla 2)
    if (biblioteca_abierta) {
        if (profes_esperando > 0) {
            pthread_cond_broadcast(&cond_profesores);
        } else {
            pthread_cond_broadcast(&cond_estudiantes);
        }
    }

    pthread_mutex_unlock(&mutex);
}

/*
 * hilo_usuario
 * - Rutina que ejecuta cada estudiante o profesor creado.
 */
void *hilo_usuario(void *arg) {
    usuario_t *usr = (usuario_t *)arg;
    
    entrar_biblioteca(usr->id, usr->es_profesor);
    sleep(2); // Tiempo de lectura dentro de la biblioteca (Apartado A)
    salir_biblioteca(usr->id, usr->es_profesor);
    
    free(usr);
    return NULL;
}

/*
 * hilo_vacaciones (Apartado B - Regla 4)
 * - Controla cíclicamente el estado de apertura/cierre de la biblioteca de forma indefinida.
 */
void *hilo_vacaciones(void *arg) {
    while (1) {
        // 1. Permanece abierta 4 segundos
        sleep(4);
        
        pthread_mutex_lock(&mutex);
        biblioteca_abierta = 0;
        printf("Library is closing for vacation.\n");
        pthread_mutex_unlock(&mutex);

        // 2. Permanece cerrada por vacaciones durante 2 segundos
        sleep(2);

        pthread_mutex_lock(&mutex);
        biblioteca_abierta = 1;
        printf("Library is now open after vacation.\n");
        
        // Al reabrir, despertamos en cascada priorizando profesores
        if (profes_esperando > 0) {
            pthread_cond_broadcast(&cond_profesores);
        } else {
            pthread_cond_broadcast(&cond_estudiantes);
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fichero_input>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("Error al abrir el archivo input.txt");
        return EXIT_FAILURE;
    }

    int total_usuarios;
    if (fscanf(fp, "%d", &total_usuarios) != 1) {
        fprintf(stderr, "Error al leer el numero total de usuarios\n");
        fclose(fp);
        return EXIT_FAILURE;
    }

    // Inicialización del Monitor
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_estudiantes, NULL);
    pthread_cond_init(&cond_profesores, NULL);
    pthread_cond_init(&cond_vacaciones, NULL);

    pthread_t hilos[total_usuarios];
    pthread_t th_vacaciones;

    // [Apartado B] Creamos el hilo de control vacacional
    pthread_create(&th_vacaciones, NULL, hilo_vacaciones, NULL);

    // Lectura de los usuarios del archivo y lanzamiento secuencial
    int tipo, id_usr = 0;
    while (id_usr < total_usuarios && fscanf(fp, "%d", &tipo) == 1) {
        usuario_t *usr = malloc(sizeof(usuario_t));
        usr->id = id_usr;
        usr->es_profesor = tipo; // 1 = Profesor, 0 = Estudiante

        pthread_create(&hilos[id_usr], NULL, hilo_usuario, usr);
        id_usr++;
        usleep(10000); // Pequeño retardo de entrada para ordenar la traza de llegada
    }
    fclose(fp);

    // Esperamos a que todos los usuarios completen su lectura y salgan
    for (int i = 0; i < total_usuarios; i++) {
        pthread_join(hilos[i], NULL);
    }

    // [Apartado B] Una vez que no queda nadie, cancelamos el bucle del hilo vacacional
    pthread_cancel(th_vacaciones);
    pthread_join(th_vacaciones, NULL);

    // Destrucción de recursos del monitor
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_estudiantes);
    pthread_cond_destroy(&cond_profesores);
    pthread_cond_destroy(&cond_vacaciones);

    return EXIT_SUCCESS;
}