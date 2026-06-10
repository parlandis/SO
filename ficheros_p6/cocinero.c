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

// Nota de ejecución externa: para lanzar los procesos consumidores:
// for i in $(seq 10) ; do ./salvajes & done

#define MAX_BUFFER 1024       /* Tamaño base del buffer */
#define M 10                  /* Número de porciones máximas que caben en la olla */

// Declaración de semáforos POSIX con nombre y variables compartidas globales
sem_t *empty, *full;          // Punteros a los semáforos de control
int *pot;                     // Puntero entero a la memoria compartida (la olla)
volatile sig_atomic_t finish = 0; // Bandera segura para terminar el bucle de ejecución de forma limpia

/*
 * putServingsInPot
 * - Simula la acción del cocinero reponiendo la comida.
 * - Escribe el número total de porciones directamente en la memoria RAM compartida (*pot).
 */
void putServingsInPot(int servings) {
    printf("[COCINERO %d] Cocinando %d porciones...\n", getpid(), servings);
    *pot = servings; // Modificación del recurso compartido
    printf("[COCINERO %d] Olla llena. Porciones actuales: %d\n", getpid(), *pot);
}

/*
 * sem_wait
 * - Realiza la operación P (proberen / comprobar) o "bajar" sobre un semáforo.
 * - Decrementa en 1 el contador del semáforo de forma atómica.
 * - Si el valor actual es 0, bloquea/duerme al proceso llamador hasta que cambie.
 */
void cook(void)
{
    while(!finish) { // Bucle infinito de cocinado hasta que salte un Ctrl+C
        printf("[COCINERO %d] Esperando a que los salvajes pidan comida...\n", getpid());
        
        // El cocinero se duerme aquí. Solo despierta cuando los salvajes dejen la olla a 0
        if (sem_wait(empty) == -1) { 
            // Si es interrumpido por una señal, sem_wait devuelve -1 y pone errno en EINTR
            if (errno == EINTR) continue; 
            perror("[COCINERO] Error en sem_wait(empty)");
            return; 
        }
        
        // Si finish cambió a 1 durante el bloqueo, salimos sin cocinar
        if (finish) break;

        putServingsInPot(M); // Sección Crítica: Rellena la olla con M porciones
        
        // Realiza la operación V (verhogen) sobre 'full'. Incrementa el semáforo avisando que hay comida
        sem_post(full); 
    }
}

/*
 * handler
 * - Manejador de señales asíncronas para SIGINT (Ctrl+C) y SIGTERM.
 * - Activa la bandera de terminación y realiza el unlink definitivo de los recursos compartidos.
 */
void handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM){ 
        finish = 1; // Rompe el bucle de cook()
        printf("\n[COCINERO %d] Señal de salida recibida (%s). Finalizando...\n", 
               getpid(), signo == SIGINT ? "SIGINT" : "SIGTERM");
    }
    
    // CORRECCIÓN DE BUG: Desmapear y eliminar de la RAM usando los mismos nombres de apertura
    if (pot != MAP_FAILED && pot != NULL) {
        munmap(pot, sizeof(int)); // Deshace el puente virtual hacia la memoria compartida
    }
    shm_unlink("/BUFFER");       // Elimina el contenedor de memoria del sistema
    
    if (empty != SEM_FAILED && empty != NULL) sem_close(empty); // Cierra el descriptor del semáforo
    if (full != SEM_FAILED && full != NULL) sem_close(full);
    
    sem_unlink("/EMPTY");        // Elimina el objeto semáforo vacío del Kernel
    sem_unlink("/FULL");         // Elimina el objeto semáforo lleno del Kernel
	sem_unlink("/SAVAGES");
    
    exit(EXIT_SUCCESS);          // Terminamos el proceso de forma segura
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

    // Activamos los manejadores en el Kernel de Linux
    if (sigaction(SIGTERM, &term, NULL) == -1 || sigaction(SIGINT, &inter, NULL) == -1) {
        perror("[COCINERO] Error al establecer manejadores de señal");
        exit(EXIT_FAILURE);
    }

    /*
     * sem_open
     * - Inicializa o abre un semáforo con nombre POSIX público para el sistema operativo.
     * - El parámetro 'name' debe empezar por una barra '/'.
     * - 'oflag' determina si se crea (O_CREAT).
     * - 'mode' define los permisos UNIX de lectura y escritura (0644).
     * - 'value' establece el valor inicial del contador interno del semáforo.
     */
    // CORRECCIÓN: Los nombres lógicos deben coincidir con los que borramos en el unlinked (/EMPTY y /FULL)
    empty = sem_open("/EMPTY", O_CREAT, 0644, 0); // 1 = La olla empieza vacía, el cocinero debe cocinar ya
    full = sem_open("/FULL", O_CREAT, 0644, 0);   // 0 = No hay porciones disponibles inicialmente

    if(empty == SEM_FAILED || full == SEM_FAILED){
        fprintf(stderr, "[COCINERO] Error crítico al crear semáforos\n");
        if (empty != SEM_FAILED) sem_unlink("/EMPTY");
        if (full != SEM_FAILED) sem_unlink("/FULL");
        exit(EXIT_FAILURE);
    }

    /*
     * shm_open
     * - Abre o crea un objeto de memoria compartida POSIX en la RAM (sin tocar el disco duro).
     * - Su ruta física virtual es visible en sistemas Linux bajo el directorio '/dev/shm/'.
     * - 'oflag' configura el modo (O_CREAT para fundarlo, O_RDWR para poder leer/escribir en él).
     * - 'mode' define los permisos de usuario habituales (S_IRUSR | S_IWUSR).
     * - Devuelve un descriptor numérico (int) que actúa igual que el descriptor de un archivo.
     */
    int shd;
    shd = shm_open("/BUFFER", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR); //
    if(shd == -1){
        perror("[COCINERO] Error al abrir objeto de memoria compartida shm_open");
        sem_unlink("/EMPTY");
        sem_unlink("/FULL");
        return EXIT_FAILURE;
    }

    /*
     * ftruncate
     * - Modifica de forma exacta el tamaño del objeto asignado por un descriptor.
     * - Al crearse con shm_open, los bloques de memoria compartida nacen midiendo 0 bytes.
     * - Recibe el descriptor (shd) y el tamaño total deseado medido en bytes (sizeof(int)).
     * - Devuelve 0 si tiene éxito o -1 en caso de fallo.
     */
    if(ftruncate(shd, sizeof(int)) == -1){   //
        perror("[COCINERO] Error al ajustar el tamaño con ftruncate");
        sem_unlink("/EMPTY"); sem_unlink("/FULL"); shm_unlink("/BUFFER");
        close(shd);
        exit(EXIT_FAILURE);
    }

    /*
     * mmap
     * - Mapea (crea un puente virtual) entre el objeto de memoria del Kernel y tu proceso.
     * - Permite acceder a la memoria compartida mediante un puntero convencional de C.
     * - 'addr': NULL para dejar que el Kernel elija la dirección virtual automáticamente.
     * - 'length': Tamaño del segmento a mapear en bytes (sizeof(int)).
     * - 'prot': Protecciones del puntero (PROT_WRITE / PROT_READ).
     * - 'flags': MAP_SHARED indica que las escrituras serán visibles por los demás procesos inmediatamente.
     * - 'fd' y 'offset': El descriptor del shm y la posición de inicio (0).
     */
    pot = (int *) mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shd, 0);
    if(pot == MAP_FAILED){
        perror("[COCINERO] Error al realizar el mapeo mmap");
        sem_unlink("/EMPTY"); sem_unlink("/FULL"); shm_unlink("/BUFFER");
        close(shd);
        exit(EXIT_FAILURE);
    }


    // Una vez mapeado el puntero, podemos cerrar el descriptor numérico de forma segura
    close(shd); 

    printf("[COCINERO %d] Semáforos y memoria compartida correctamente creados\n", getpid());
    
    // Entramos en la rutina de cocinado síncrono
    cook();

    return 0;
}

/*
 * ====================================================================================
 * DOCUMENTACIÓN GLOBAL: FUNCIONAMIENTO DE SEMÁFOROS E INTERPROCESO (IPC)
 * ====================================================================================
 *
 * 1. TEORÍA GENERAL DE SEMÁFOROS CON NOMBRE (POSIX Named Semaphores)
 * ------------------------------------------------------------------------------------
 * Un semáforo con nombre es un contador entero síncrono gestionado de forma nativa por el Kernel.
 * Al tener un identificador textual con barra inicial (ej: "/EMPTY"), cualquier proceso 
 * independiente del sistema operativo puede abrirlo para coordinar el acceso a un recurso común.
 * * Funciona mediante dos operaciones fundamentales y atómicas (no pueden ser interrumpidas):
 *
 * - sem_wait(sem) [Operación P]: Resta 1 al contador interno.
 * - Si el valor resultante es mayor o igual a 0, el proceso sigue adelante sin detenerse.
 * - Si el valor era 0, el proceso se bloquea (se duerme) en una cola de espera del Kernel.
 * * - sem_post(sem) [Operación V]: Suma 1 al contador interno.
 * - Si había procesos durmiendo en la cola de ese semáforo, el Kernel despierta a uno de ellos
 * para que reanude su marcha de inmediato.
 *
 * En este problema ("Los salvajes antropófagos"), los semáforos implementan una sincronización cruzada:
 * - El cocinero espera en 'empty' (inicializado en 1). Cocina solo cuando la olla se vacía del todo.
 * - Los salvajes esperan en 'full' (inicializado en 0). Solo comen si el cocinero avisa que la olla se llenó.
 /*
 * 2. FUNCIONAMIENTO COMPLETO DEL FLUJO DEL CÓDIGO
 * ------------------------------------------------------------------------------------
 * El programa representa al productor (Cocinero) en un esquema clásico de productor-consumidor
 * modificado para un recurso que se consume por partes pero se repone entero de golpe.
 *
 * FASE 1: Inicialización de la Infraestructura Compartida
 * 1. El programa principal blinda los canales con sigaction para interceptar un Ctrl+C.
 * 2. Abre/Crea los dos semáforos públicos globales: "/EMPTY" y "/FULL".
 * 3. Crea un objeto de memoria compartida llamado "/BUFFER" usando shm_open().
 * 4. Como nace midiendo 0 bytes, le da tamaño dinámico exacto de un entero con ftruncate().
 * 5. Mediante mmap(), conecta la RAM del Kernel con el puntero local '*pot'. Todo lo que se escriba
 * en '*pot' cambiará instantáneamente para cualquier otro proceso en el sistema.
 *
 * FASE 2: El Bucle de Coordinación Asíncrona (cook)
 * 1. El Cocinero entra en cook() y se ejecuta la línea sem_wait(empty). Al valer 'empty' 1 originalmente,
 * baja a 0 y pasa de largo.
 * 2. Invoca putServingsInPot(10), poniendo la olla virtual con 10 porciones compartidas en la RAM.
 * 3. Llama a sem_post(full). Incrementa 'full' a 1, indicando a los procesos "salvajes" independientes
 * que ya hay comida disponible en el sistema.
 * 4. El bucle da la vuelta y el cocinero se estampa contra el siguiente 'sem_wait(empty)'. Como 'empty'
 * ahora vale 0, el cocinero se queda **completamente congelado en suspensión** en esa línea.
 * 5. Mientras el cocinero duerme, los procesos "salvajes" entran, decrementan las porciones de '*pot' una
 * a una. Cuando el último salvaje ve que '*pot == 0', ejecuta un 'sem_post(empty)'.
 * 6. Ese post despierta de inmediato al cocinero en el Kernel, repitiendo el ciclo de llenado infinito.
 *
 * FASE 3: Desmantelamiento y Limpieza de Basura (handler)
 * Si el usuario pulsa Ctrl+C, el flujo salta al manejador. Es de vital importancia en IPC limpiar todo:
 * - Se hace munmap() para romper el enlace con el puntero.
 * - Se ejecutan sem_close() y shm_unlink() / sem_unlink(). Si no se hiciera el unlink, tanto la memoria
 * compartida como los semáforos se quedarían flotando "vivos" consumiendo recursos en la RAM de Linux 
 * incluso tras haber cerrado el programa, corrompiendo futuras ejecuciones.
 */

 /*
 * ====================================================================================
 * DOCUMENTACIÓN GLOBAL: ANÁLISIS DEL BUG DE INTERBLOQUEO (DEADLOCK INICIAL)
 * ====================================================================================
 * * ¿QUÉ SUCEDÍA EN TU EJECUCIÓN CONGELADA?
 * Al ejecutar `./cocinero`, se mostraba el mensaje de creación correcta pero se quedaba en:
 * "[COCINERO X] Esperando a que los salvajes pidan comida..."
 * Al lanzar el bucle bash de salvajes, estos imprimían que se conectaban con 0 porciones,
 * decían "quiere comida", y el programa se congelaba por completo sin avanzar.
 * * EL MOTIVO TÉCNICO (Persistencia en el Kernel de Recursos POSIX):
 * Los semáforos POSIX con nombre (`/EMPTY`, `/FULL`, `/SAVAGES`) tienen persistencia en el Kernel. 
 * Esto significa que si en una ejecución anterior el programa se interrumpe de forma abrupta 
 * (mediante Ctrl+C, fallos de segmento o matando procesos en background), los semáforos 
 * NO se destruyen; permanecen guardados en la memoria interna del sistema operativo con el 
 * último valor que tenían (habitualmente 0 debido a procesos que se quedaron bloqueados en ellos).
 * * Cuando volvías a arrancar `./cocinero`, el código invocaba:
 * `sem_open("/EMPTY", O_CREAT, 0644, 1);`
 * La especificación POSIX determina que si se usa la bandera `O_CREAT` pero el semáforo YA EXISTE 
 * en el sistema, el Kernel ignora por completo el último parámetro del valor inicial (el `1`). 
 * En su lugar, abre el semáforo existente con el valor residual que tuviera (que era `0`).
 * * CONSECUENCIA EN CADENA (Efecto Dominó):
 * 1. El Cocinero abría `/EMPTY` creyendo que valía 1. Al ejecutar `sem_wait(empty)`, en lugar de 
 * pasar de largo y cocinar, el valor bajaba a -1 (o se quedaba en 0 bloqueante) congelando al 
 * cocinero de inmediato en la salida de inicio.
 * 2. Los Salvajes arrancaban en paralelo. El primer salvaje intentaba adquirir el semáforo `/SAVAGES` (m).
 * Como este semáforo también arrastraba basura o no estaba sincronizado desde el cocinero, 
 * su valor en el Kernel era `0`.
 * 3. El salvaje ejecutaba `sem_wait(m)` para entrar en exclusión mutua, y al valer `0`, se quedaba 
 * congelado inmediatamente en el Kernel antes de poder evaluar si la olla estaba vacía o enviar
 * la señal de aviso. Todos los demás salvajes de la cola bash (`seq 10`) hacían lo mismo, 
 * generando un interbloqueo totalizado donde nadie despertaba a nadie.
 * * CÓMO SE CORRIGIÓ:
 * Colocando un bloque de limpieza proactiva con `sem_unlink` y `shm_unlink` al inicio del `main` 
 * del cocinero. Esto purga por completo cualquier estructura zombi del Kernel de Linux antes de empezar, 
 * asegurando que el sistema operativo inicialice los contadores desde cero absoluto en cada ejecución limpia.
 */