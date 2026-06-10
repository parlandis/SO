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


sem_t *dock;
int * memC; // memoria compartida  
int finish;

void handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM){ 
        finish = 1; // Rompe el bucle de cook()
        printf("\n[CONTROL %d] Señal de salida recibida (%s). Finalizando...\n", 
               getpid(), signo == SIGINT ? "SIGINT" : "SIGTERM");
    }
    exit(EXIT_SUCCESS);          // Terminamos el proceso de forma segura
}

int main(int argc, char *argv[]){
    int N; // Num camiones



    if(argc < 2){
        fprintf(stderr, "Falta el numero de camiones\n");
        return EXIT_FAILURE;
    }


        // Configuración de las estructuras sigaction para captura de señales
    struct sigaction term = { 0 }, inter = { 0 };
    term.sa_handler = handler; 
    term.sa_flags = 0;
    sigemptyset(&term.sa_mask); // Máscara vacía para no bloquear otras señales mientras corre el handler

    inter.sa_handler = handler; 
    inter.sa_flags = 0;
    sigemptyset(&inter.sa_mask);

    // Activamos los manejadores en el Kernel de Linux
    if (sigaction(SIGTERM, &term, NULL) == -1 || sigaction(SIGINT, &inter, NULL) == -1) {
        perror("[COCINERO] Error al establecer manejadores de señal");
        exit(EXIT_FAILURE);
    }


    N = atoi(argv[1]);

    if(N < 0){
        fprintf(stderr, "El numero debe ser positivo\n");
        return EXIT_FAILURE;
    }
    sem_unlink("/sem_loading_docks");


    dock = sem_open("/sem_loading_docks", O_CREAT, 0644, N);

    if(dock == SEM_FAILED){
        fprintf(stderr, "[CONTROL] Error al inicializar el semaforo\n");
        return EXIT_FAILURE;
    }

    int shd;
    shd = shm_open("/BUFFER", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR); //
    if(shd == -1){
        perror("[CONTROL] Error al abrir objeto de memoria compartida shm_open");
        sem_unlink("/sem_loading_docks");
        return EXIT_FAILURE;
    }

    if(ftruncate(shd, sizeof(int)) == -1){   //
        perror("[COCINERO] Error al ajustar el tamaño con ftruncate");
        sem_unlink("/EMPTY"); sem_unlink("/FULL"); shm_unlink("/BUFFER");
        close(shd);
        exit(EXIT_FAILURE);
    }
    
    printf("[CONTROL] “System initialized with %d loading docks. \n", N);
    sleep(10);

    while(finish != 1){

    }

    shm_unlink("/BUFFER");
    close(shd);
    sem_unlink("/sem_loading_docks");
    return 0;
}
