#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt */
#include <string.h>     /* strlen */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <sys/wait.h>   // waitpid, macros WIFSIGNALED, WTERMSIG
#include <sys/time.h>   // gettimeofday
#include <sys/resource.h> // getrusage

// Función de ayuda de la plantilla
void timeval_diff(struct timeval *result, struct timeval *end, struct timeval *start) {
    result->tv_sec = end->tv_sec - start->tv_sec;
    result->tv_usec = end->tv_usec - start->tv_usec;
    if (result->tv_usec < 0) {
        result->tv_sec--;
        result->tv_usec += 1000000;
    }
}

#define TIMEVAL_TO_SECS(tv) ((double)(tv).tv_sec + (double)(tv).tv_usec / 1000000.0)

int run_command(char *cmd){
    struct sigaction sa_ignore, sa_default;
    
    // 1. CONFIGURAR PADRE: Ignorar SIGINT y SIGTERM
    sa_ignore.sa_handler = SIG_IGN;
    sa_ignore.sa_flags = 0;
    sigemptyset(&sa_ignore.sa_mask);
    sigaction(SIGINT, &sa_ignore, NULL);
    sigaction(SIGTERM, &sa_ignore, NULL);

    pid_t pid = fork();
    if(pid < 0){
        perror("Error en fork");
        exit(EXIT_FAILURE);
    } 
    else if(pid == 0){
        // 2. CONFIGURAR HIJO: Restaurar comportamiento por defecto (SIG_DFL)
        sa_default.sa_handler = SIG_DFL;
        sa_default.sa_flags = 0;
        sigemptyset(&sa_default.sa_mask);
        sigaction(SIGINT, &sa_default, NULL);
        sigaction(SIGTERM, &sa_default, NULL);

        // REQUISITO APARTADO A: Equivalente a ejecutar "/bin/sh -c cmd"
        // Creamos el vector de argumentos para pasárselo a execvp
        char *args[] = {"/bin/sh", "-c", cmd, NULL};
        
        execvp(args[0], args);
        perror("Error en execvp");
        exit(EXIT_FAILURE);
    }

    // El padre informa del PID del hijo (Requisito Apartado a)
    printf("child process pid %d\n", pid);

    int status;
    // Esperamos al hijo. Si se interrumpe por una señal propia, controlamos EINTR
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("Error en waitpid");
            exit(EXIT_FAILURE);
        }
    }

    // REQUISITO APARTADO B: Comprobar si terminó por una señal
    if (WIFSIGNALED(status)) {
        printf("child with pid %d terminated by signal %d\n", pid, WTERMSIG(status));
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if(argc < 2){
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct timeval start, end, diff;
    struct rusage usage;

    // Medición del tiempo real (Antes)
    gettimeofday(&start, NULL); 

    // Ejecución del comando
    run_command(argv[1]);

    // Medición del tiempo real (Después)
    gettimeofday(&end, NULL);

    // REQUISITO APARTADO C: Uso de getrusage para estadísticas de los hijos
    if (getrusage(RUSAGE_CHILDREN, &usage) < 0) {
        perror("Error en getrusage");
        return EXIT_FAILURE;
    }

    // Calculamos e imprimimos la diferencia de tiempo real
    timeval_diff(&diff, &end, &start);
   
    printf("real: %.3f s\n", TIMEVAL_TO_SECS(diff));
    printf("user: %.3f s\n", TIMEVAL_TO_SECS(usage.ru_utime));
    printf("sys:  %.3f s\n", TIMEVAL_TO_SECS(usage.ru_stime));

    return 0;
}