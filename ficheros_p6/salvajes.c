#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* Para constantes de modos de permisos */
#include <fcntl.h>           /* Para constantes O_CREAT, O_RDONLY... */
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <sys/wait.h>
#include <semaphore.h>

#define MAX_BUFFER 1024 
#define NUMITER 3            // Número de iteraciones que cada salvaje realiza para comer

// Declaración de estructuras de control compartidas
sem_t *empty, *full, *m;     // Semáforos compartidos con el cocinero
int* pot;                    // Puntero a la memoria compartida de la olla

/*
 * getServingsFromPot
 * - Accede a la dirección de memoria compartida apuntada por pot.
 * - Aplica los paréntesis correctos para decrementar de forma atómica el valor entero.
 * - Resta una ración a la olla común.
 */
void getServingsFromPot(void)
{
    // SOLUCIÓN AL BUG 1: Forzamos con paréntesis que se reste el valor, no la dirección
    (*pot)--; 
}

/*
 * eat
 * - Llama a la función de extracción de porciones.
 * - Imprime un mensaje por pantalla con el identificador del proceso actual (PID).
 * - Simula el tiempo de masticación durmiendo el proceso de 0 a 4 segundos de forma aleatoria.
 */
void eat(void)
{
    getServingsFromPot();
    unsigned long id = (unsigned long) getpid();
    printf("[SALVAJE %lu] Comiendo una ración... Quedan %d en la olla.\n", id, *pot);
    sleep(rand() % 5);
}

/*
 * savages
 * - Ejecuta el bucle principal de control del proceso consumidor.
 * - Sincroniza mediante exclusión mutua el acceso a la olla para evitar condiciones de carrera.
 * - Despierta al cocinero mediante señales POSIX si detecta que el plato está vacío.
 */
void savages(void)
{
	printf("[SALVAJE %d] quiere comida\n", (int) getpid());
    for (int i = 0; i < NUMITER; i++) {
        
        // Pide el testigo del semáforo 'm' (Mutex). Solo un salvaje a la vez puede examinar la olla
        sem_wait(m);
        
        // Sección Crítica de control
        if (*pot <= 0) {
            printf("[SALVAJE %d] ¡La olla está vacía! Pidiendo comida al cocinero...\n", (int) getpid());
            
            // Operación V (subir): Despierta al cocinero que está atrapado en su sem_wait(empty)
            sem_post(empty);

            // Operación P (bajar): El salvaje se duerme aquí reteniendo el mutex 'm'. 
            // Esto es correcto porque evita que otros salvajes saturen al cocinero con peticiones falsas.
            sem_wait(full);
        }
        
        printf("[SALVAJE %d] Accediendo a comer. Estado de la olla: %d porciones.\n", (int) getpid(), *pot);
        eat(); // Extrae la ración y come
        
        // Operación V (subir): Libera el cerrojo de la olla para que entre el siguiente salvaje concurrente
        sem_post(m);
        
        // Simula un pequeño retraso fuera de la sección crítica antes de volver a tener hambre
        usleep(100000); 
    }
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv; // Evita avisos de variables no usadas
    srand(getpid());        // Inicializa la semilla aleatoria de forma única por proceso

    /*
     * sem_open
     * - Abre o crea un semáforo con nombre POSIX público para el sistema operativo.
     * - El parámetro "/SAVAGES" se abre con O_CREAT para asegurar su existencia inicializada en 1.
     * - "/EMPTY" y "/FULL" se abren con el flag '0' (modo cliente) indicando que dependemos 
     * de que el proceso cocinero los haya creado previamente en memoria.
     * - Devuelve un puntero sem_t* o SEM_FAILED si el cocinero no está en marcha.
     */
    m = sem_open("/SAVAGES", O_CREAT, 0777, 1);  // Cierre mutuo de la olla (1 = Libre)
    empty = sem_open("/EMPTY", 0);
    full = sem_open("/FULL", 0);

    // Validación de seguridad obligatoria de IPC
    if(empty == SEM_FAILED || full == SEM_FAILED || m == SEM_FAILED){
        fprintf(stderr, "[SALVAJE %d] ERROR CRÍTICO: No se encuentran los semáforos en la RAM. ¡Arranca primero el cocinero!\n", getpid());
        exit(EXIT_FAILURE);
    }

    /*
     * shm_open
     * - Se conecta a un objeto de memoria compartida ya existente en la RAM del sistema.
     * - "/BUFFER" es la ruta virtual compartida (mapeada internamente en /dev/shm/).
     * - O_RDWR: Solicita permisos de lectura y escritura sobre la zona compartida.
     * - Devuelve el descriptor numérico de fichero entero (int).
     */
    int shd;
    shd = shm_open("/BUFFER", O_RDWR, 0);
    if(shd == -1){
        perror("[SALVAJE] Error fatal al mapear el descriptor con shm_open");
        sem_close(empty); sem_close(full); sem_close(m);
        return EXIT_FAILURE;
    }

    /*
     * mmap
     * - Mapea (construye el puente lógico) entre la región de memoria del Kernel y este proceso.
     * - Permite que las modificaciones hechas a través del puntero '*pot' afecten directamente a la RAM común.
     * - PROT_READ | PROT_WRITE: Configura que el puntero puede leer y alterar los datos de la olla.
     * - MAP_SHARED: Obliga a que los cambios se propaguen en tiempo real a los demás procesos concurrentes.
     */
    pot = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shd, 0);
    if(pot == MAP_FAILED){
        perror("[SALVAJE] Error al realizar la proyección de memoria mmap");
        sem_close(empty); sem_close(full); sem_close(m);
        close(shd);
        exit(EXIT_FAILURE);
    }

    // Una vez que mmap tiene éxito, el descriptor de entero ya no se necesita abierto
    close(shd);

    printf("[SALVAJE %d] Conectado a la olla. Porciones disponibles: %d\n", getpid(), *pot);
    
    // Entramos a la rutina de consumo de porciones
    savages();

    // =================================================================
    // FASE DE LIMPIEZA DEL PROCESO CLIENTE (SALVAJE)
    // =================================================================
    // SOLUCIÓN AL BUG 2: Los clientes SOLO cierran descriptores locales. NUNCA hacen unlink.
    sem_close(empty);
    sem_close(full);
    sem_close(m);
    
    // SOLUCIÓN AL BUG 1 (Efecto secundario): Desmapeamos pasándole la dirección original intacta
    munmap(pot, sizeof(int));
    
    printf("[SALVAJE %d] Saciado. Recursos cerrados correctamente en este proceso.\n", getpid());

    return 0;
}

/*
 * ====================================================================================
 * DOCUMENTACIÓN GLOBAL: SINCRONIZACIÓN Y ARQUITECTURA DE PROCESOS CLIENTE (SALVAJES)
 * ====================================================================================
 *
 * 1. DISEÑO E INTERACCIÓN DE LOS SEMÁFOROS (Mecánica Inter-Proceso IPC)
 * ------------------------------------------------------------------------------------
 * Este programa modela un entorno clásico de "Múltiples Consumidores" frente a un único
 * "Productor" (Cocinero), coordinados en un espacio de memoria física compartida.
 * Para evitar que los procesos colisionen destruyendo los datos, se implementan tres capas:
 *
 * - Semáforo "/SAVAGES" (m): Actúa como un Mutex binario (inicializado en 1). Su función es
 * garantizar la EXCLUSIÓN MUTUA entre los salvajes. Evita que dos salvajes miren la olla,
 * vean que queda 1 porción, entren a la vez, y dejen el contador en un valor inválido (-1).
 *
 * - Semáforo "/EMPTY": Controla la condición de olla vacía. Inicializado en 1 por el cocinero,
 * sirve para suspender al cocinero. Cuando un salvaje pone la olla a 0, hace un sem_post(empty)
 * para activar el fuego del cocinero.
 *
 * - Semáforo "/FULL": Controla la condición de comida lista. Inicializado en 0. El salvaje
 * que encuentra la olla vacía se duerme aquí con sem_wait(full) liberando la CPU hasta que
 * el cocinero termina de reponer las 10 raciones de golpe.
 */