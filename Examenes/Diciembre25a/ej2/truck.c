#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* Para constantes de modos de permisos */
#include <fcntl.h>           /* Para constantes O_CREAT, O_RDWR... */
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>         /* Librería obligatoria para trabajar con hilos */

sem_t *dock;
int currentLoad;             /* Variable global compartida por los hilos (carga actual) */

pthread_mutex_t mutex;       /* Cerrojo de exclusión mutua para proteger la variable global */

// Estructura para pasar los parámetros a cada hilo operario
typedef struct {
    int paq;
    int id; 
    int maxP;
} operario_t;

/*
 * cargar
 * - Bucle de carga concurrente que realiza cada hilo de manera individual.
 */
void cargar(int p, int id){
    for (int i = 0; i < p; i++) {
        // [APARTADO C] Cada operario hace una llamada a usleep(1000) por paquete fuera de la sección crítica
        usleep(1000);
        
        // [APARTADO C] Sección Crítica: Protegemos el incremento de la variable global compartida
        pthread_mutex_lock(&mutex);
        currentLoad++;
        pthread_mutex_unlock(&mutex);
    }
}

/*
 * operario
 * - Función de inicio para los hilos pthread.
 */
void *operario(void *arg){
    operario_t *oper = (operario_t *) arg;
    int id = oper->id, p = oper->paq;
    
    // Ejecuta la rutina de carga asignada
    cargar(p, id);
    
    free(oper); // Liberamos la memoria de la estructura asignada a este hilo
    return NULL;
}

/*
 * main
 * - Controla el flujo del camión: solicita muelle, lanza operarios concurrentes y libera el muelle.
 */
int main(int argc, char *argv[]){
    int id, packages;
    
    // Validación de los argumentos obligatorios del camión
    if(argc < 3){
        fprintf(stderr, "Uso: %s <id> <packages>\n", argv[0]);
        return EXIT_FAILURE;
    }

    id = atoi(argv[1]);
    packages = atoi(argv[2]);

    if(packages < 0){
        fprintf(stderr, "El numero de paquetes debe ser positivo\n");
        return EXIT_FAILURE;
    }

    // [APARTADO B] Abrimos el semáforo con nombre creado previamente por control
    dock = sem_open("/sem_loading_docks", 0);

    if(dock == SEM_FAILED){
        perror("[TRUCK] Error al abrir el semaforo /sem_loading_docks");
        return EXIT_FAILURE;
    }

    // [CORRECCIÓN] Se eliminó el bloque de shm_open y mmap sobrante que no pide el enunciado

    // [APARTADO B] Traza obligatoria ANTES de solicitar muelle
    printf("[Truck %d] Waiting for a loading dock...\n", id);
    fflush(stdout);

    // Decrementamos el semáforo. Si vale 0, el proceso camión se bloquea aquí
    if(sem_wait(dock) == -1){
        perror("[TRUCK] Error en sem_wait");
        sem_close(dock);
        return EXIT_FAILURE;
    }
    
    // [APARTADO B] Traza obligatoria TRAS conseguir la plaza en el muelle
    printf("[Truck %d] Entering the loading dock.\n", id);
    fflush(stdout);

    // [APARTADO C] Inicializamos el mutex de exclusión mutua para controlar la carga global
    pthread_mutex_init(&mutex, NULL);

    pthread_t op1, op2;
    
    // División equitativa de los paquetes entre los dos operarios (hilos)
    int p1 = packages / 2;
    int p2 = packages - p1;

    // Reserva de memoria dinámica para los argumentos de los hilos
    operario_t *oper1 = malloc(sizeof(operario_t));
    oper1->id = 1;
    oper1->maxP = packages;
    oper1->paq = p1;
    
    operario_t *oper2 = malloc(sizeof(operario_t));
    oper2->id = 2;
    oper2->maxP = packages;
    oper2->paq = p2;
    
    currentLoad = 0; // Inicializamos la carga del camión a 0 antes de empezar

    // [APARTADO C] Creamos los dos hilos operarios concurrentes
    if(pthread_create(&op1, NULL, operario, oper1) != 0){
        fprintf(stderr, "Error al crear operario 1\n");
        sem_post(dock); sem_close(dock);
        return EXIT_FAILURE;
    }
    
    if(pthread_create(&op2, NULL, operario, oper2) != 0){
        fprintf(stderr, "Error al crear operario 2\n");
        sem_post(dock); sem_close(dock);
        return EXIT_FAILURE;
    }

    // [APARTADO C] El proceso principal espera obligatoriamente a que terminen ambos operarios
    pthread_join(op1, NULL);
    pthread_join(op2, NULL);

    // [APARTADO C] Mensaje final idéntico al formato exigido por el enunciado
    printf("[Truck %d] Loading finished (Total loaded: %d). Exiting.\n", id, currentLoad);
    fflush(stdout);
    
    // [CORRECCIÓN] Destruimos el mutex de los hilos para no dejar fugas de recursos en el sistema
    pthread_mutex_destroy(&mutex);

    // [APARTADO B/C] Incrementamos el semáforo para liberar el muelle y permitir el acceso a otro camión
    sem_post(dock);
    
    // Cerramos el descriptor local del semáforo
    sem_close(dock);

    return EXIT_SUCCESS;
}