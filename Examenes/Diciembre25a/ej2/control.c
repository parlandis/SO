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
int finish;

/*
 * handler
 * - Manejador de señales para capturar SIGINT y SIGTERM si fuera necesario.
 */
void handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM){ 
        finish = 1; 
        printf("\n[CONTROL %d] Señal de salida recibida. Finalizando...\n", getpid());
    }
    exit(EXIT_SUCCESS);          
}

/*
 * main
 * - Procesa el argumento N, limpia residuos y crea el semáforo con nombre.
 * - [CORRECCIÓN] Se elimina el bucle infinito y el sleep para que termine de inmediato como pide el texto.
 */
int main(int argc, char *argv[]){
    int N; // Número de muelles

    // Validación de argumentos de entrada
    if(argc < 2){
        fprintf(stderr, "Uso: %s <numero_de_muelles>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Configuración básica de señales por seguridad
    struct sigaction term = { 0 }, inter = { 0 };
    term.sa_handler = handler; 
    term.sa_flags = 0;
    sigemptyset(&term.sa_mask);

    inter.sa_handler = handler; 
    inter.sa_flags = 0;
    sigemptyset(&inter.sa_mask);

    if (sigaction(SIGTERM, &term, NULL) == -1 || sigaction(SIGINT, &inter, NULL) == -1) {
        perror("Error al establecer manejadores de señal");
        exit(EXIT_FAILURE);
    }

    // Capturamos el argumento N
    N = atoi(argv[1]);

    if(N <= 0){
        fprintf(stderr, "El numero de muelles debe ser un entero positivo\n");
        return EXIT_FAILURE;
    }
    
    // [APARTADO A] Desvincular el semáforo para evitar residuos de ejecuciones anteriores
    sem_unlink("/sem_loading_docks");

    // [APARTADO A] Crear e inicializar el semáforo con un valor inicial de N muelles
    dock = sem_open("/sem_loading_docks", O_CREAT, 0644, N);

    if(dock == SEM_FAILED){
        perror("[CONTROL] Error al inicializar el semaforo con sem_open");
        return EXIT_FAILURE;
    }

    // [CORRECCIÓN] Se eliminó el bloque sobrante de shm_open/ftruncate que no pedía el enunciado
    // [CORRECCIÓN] Se corrigen las comillas curvadas del printf original que daban error de sintaxis en gcc
    printf("System initialized with %d loading docks.\n", N);
    
    // Cerramos el descriptor del semáforo en este proceso antes de salir
    sem_close(dock);

    // [APARTADO A] El programa principal debe terminar inmediatamente tras inicializar
    return EXIT_SUCCESS;
}