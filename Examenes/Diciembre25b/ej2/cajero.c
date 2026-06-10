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



#define MENUPRICE 10

typedef struct{
    int last_servings; // Numero de raciones que se ha comido 
    int last_payment;  // Precio total a pagar , raciones * MENUPRICE
}cuenta_t;

sem_t *libre, *pagado;       // Punteros a los semáforos de del cajero
cuenta_t *cuenta;         
volatile sig_atomic_t finish = 0; // Bandera segura para terminar el bucle de ejecución de forma limpia
cuenta_t cuentaTotal;

void cobrar(){
    printf("[CAJERO %d] collecting 1 payment for %d serving(s)\n", getpid(), cuenta->last_servings);
    cuentaTotal.last_payment += cuenta->last_payment;
    cuentaTotal.last_servings += cuenta->last_servings;
    printf("[CAJERO] Total incomes %d for %d servings served\n", cuentaTotal.last_payment, cuentaTotal.last_servings);
}

void handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM){ 
        finish = 1; // Rompe el bucle de cajero()
        printf("\n[CAJERO %d] Señal de salida recibida (%s). Finalizando...\n", 
               getpid(), signo == SIGINT ? "SIGINT" : "SIGTERM");
    }

    if(cuenta != MAP_FAILED && cuenta != NULL){
        munmap(cuenta, sizeof(cuenta_t));
    }
    shm_unlink("/CUENTA");

    if (libre != SEM_FAILED && libre != NULL) sem_close(libre); // Cierra el descriptor del semáforo
    if (pagado != SEM_FAILED && pagado != NULL) sem_close(pagado);

    
    sem_unlink("/LIBRE");
    sem_unlink("/PAGADO");
    sem_unlink("/CAJERO");


    exit(EXIT_SUCCESS);          // Terminamos el proceso de forma segura
}

void cajero(){
    sem_post(libre);
    while(!finish){
        printf("[CAJERO %d] Esperando a que algun salvaje pague\n", getpid());

        if(sem_wait(pagado) == -1){
            if(errno == EINTR) continue;
            fprintf(stderr, "[CAJERO] error en sem_wait(libre)\n");
            return;
        }

        if(finish) break;

        cobrar();
        sem_post(libre);
    }

}

int main(int argc, char *argv[])
{
        (void)argc; (void)argv; // Evita warnings de argumentos no usados

    // Configuración de las estructuras sigaction para captura de señales
    struct sigaction term = { 0 }, inter = { 0 };
    term.sa_handler = handler; 
    term.sa_flags = 0;
    sigemptyset(&term.sa_mask); // Máscara vacía para no bloquear otras señales mientras corre el handler

    inter.sa_handler = handler; 
    inter.sa_flags = 0;
    sigemptyset(&inter.sa_mask);

    libre = sem_open("/LIBRE", O_CREAT, 0644, 0);  // avisa a salvajes 
    pagado = sem_open("/PAGADO", O_CREAT, 0644, 0);  //salvajes avisan 

    if(libre == SEM_FAILED || pagado == SEM_FAILED){
        fprintf(stderr, "[CAJERO] Error crítico al crear semáforos\n");
        if (libre != SEM_FAILED) sem_unlink("/LIBRE");
        if (pagado != SEM_FAILED) sem_unlink("/PAGADO");
        exit(EXIT_FAILURE);
    }
    
    int shd2;
    shd2 = shm_open("/CUENTA", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR); //
    if(shd2 == -1){
        perror("[CAJERO] Error al abrir objeto de memoria compartida shm_open");
        sem_unlink("/LIBRE");
        sem_unlink("/PAGADO");
        return EXIT_FAILURE;
    }

    //le damos tamaño de cuenta_t
    if(ftruncate(shd2, sizeof(cuenta_t)) == -1){   //
        perror("[CAJERO] Error al ajustar el tamaño con ftruncate");
        sem_unlink("/LIBRE"); sem_unlink("/PAGADO"); shm_unlink("/CUENTA");
        close(shd2);
        exit(EXIT_FAILURE);
    }   


    cuenta = (cuenta_t *) mmap(NULL, sizeof(cuenta_t), PROT_READ | PROT_WRITE, MAP_SHARED, shd2, 0 );
    if(cuenta == MAP_FAILED){
        perror("[CAJERO] Error al realizar el mapeo mmap");
        sem_unlink("/LIBRE"); sem_unlink("/PAGADO"); shm_unlink("/CUENTA");
        close(shd2);
        exit(EXIT_FAILURE);
    }

    close(shd2);
    cuentaTotal.last_payment = 0;
    cuentaTotal.last_servings = 0;
    cuenta->last_payment = 0;
    cuenta->last_servings = 0;

    printf("[CAJERO] The cash register is now open \n");
    cajero();




    return 0;
}