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
#include <unistd.h>  // Para sleep()
#include <pthread.h> //Libreria para trabajar con hilos


sem_t *dock;
int * memC; // memoria compartida  
int currentLoad; 

pthread_mutex_t mutex;       // El "cerrojo" para la sección crítica
pthread_cond_t condOp;   

typedef struct {
    int paq;
    int id; 
    int maxP;
} operario_t;


void cargar(int p, int id){
    // El bucle da tantas vueltas como paquetes se le asignaron a ESTE operario (su parte)
    for (int i = 0; i < p; i++) {
        // 1. Simulamos el tiempo de carga (1ms) FUERA del mutex.
        // Esto permite que ambos hilos duerman/cargen EN PARALELO de verdad.
        usleep(4000);
        
        // 2. Sección crítica: protegetemos exclusivamente el incremento de la variable global
        pthread_mutex_lock(&mutex);
        currentLoad++;
        // Si quieres mantener tu traza de depuración en la terminal:
        printf("[Operario <%d>] Cargando paquete (Total camión: %d)\n", id, currentLoad);
        pthread_mutex_unlock(&mutex);
    }
}

void *operario(void *arg){
    operario_t *oper = (operario_t *) arg;
    int id = oper->id, p = oper->paq;
    
    printf("[Operario <%d>] Registrado , a cargar %d paquetes\n", id, p);
    
    // Ejecuta su bucle de carga independiente
    cargar(p, id);
    
    free(oper); // Buena práctica: liberamos la memoria dinámica reservada en el main
    return NULL;
}


int main(int argc, char *argv[]){
    int id, packages;
    if(argc < 3){
        fprintf(stderr, "[TRUCK] Hay que señalizar el id del camion y el numero de paquetes\n");
        return EXIT_FAILURE;
    }

    id = atoi(argv[1]);
    packages = atoi(argv[2]);

    if(packages < 0){
        fprintf(stderr, "[TRUCK] El numero de paquetes tiene que ser positivo\n");
        return EXIT_FAILURE;
    }




    dock = sem_open("/sem_loading_docks", 0);

    if(dock == SEM_FAILED){
        fprintf(stderr, "[TRUCK] No se encuentra el semaforo\n");
        return EXIT_FAILURE;
    }

    int shd;
    shd = shm_open("/BUFFER", O_RDWR, 0);
    if(shd == -1){
        perror("[TRUCK] Error fatal al mapear el descriptor con shm_open");
        sem_close(dock);
        return EXIT_FAILURE;
    }

    memC = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shd, 0);
    if(memC == MAP_FAILED){
        perror("[TRUCK] Error al realizar la proyección de memoria mmap");
        sem_close(dock); 
        close(shd);
        exit(EXIT_FAILURE);
    }
    printf("[TRUCK <%d>] Waiting for a loading dock...\n ", id);

    if(sem_wait(dock) == -1){
        fprintf(stderr, "[TRUCK] error en sem_wait\n");
        return EXIT_FAILURE;
    }
    printf("[TRUCK <%d>] Entering the loading dock...\n ", id);


    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condOp, NULL);

    pthread_t op1;
    pthread_t op2;
    int p1 = packages / 2;
    int p2 = packages - p1;

    operario_t *oper1 = malloc(sizeof(operario_t));
    oper1->id = 1;
    oper1->maxP = packages;
    oper1->paq = p1;
    operario_t *oper2 = malloc(sizeof(operario_t));
    oper2->id = 2;
    oper2->maxP = packages;
    oper2->paq = p2;
    currentLoad = 0;

    if(pthread_create(&op1, NULL, operario, oper1) != 0){
        fprintf(stderr, "Error al crear operario1\n");
        sem_post(dock);
        close(shd);
        sem_close(dock);
        munmap(memC, sizeof(int));
        return EXIT_FAILURE;
    }
    
    if(pthread_create(&op2, NULL, operario, oper2) != 0){
        fprintf(stderr, "Error al crear operario2\n");
        sem_post(dock);
        close(shd);
        sem_close(dock);
        munmap(memC, sizeof(int));
        return EXIT_FAILURE;
    }

    pthread_join(op1, NULL);
    pthread_join(op2, NULL);

    //sleep(0.1*packages); // simulamos la carga

    printf("[Truck <%d>] Loading finished (Total loaded: <%d>). Exiting.\n", id, currentLoad);
    sem_post(dock);
    close(shd);

    sem_close(dock);
    munmap(memC, sizeof(int));

    return 0;
}