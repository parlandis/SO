#include <stdio.h>
#include <stdlib.h>
#include <signal.h>     // Necesaria para sigaction, alarm y kill
#include <unistd.h>     // Necesaria para fork, execvp, getpid y alarm
#include <string.h>
#include <sys/types.h>  // Define tipos de datos de sistema como pid_t
#include <sys/wait.h>   // Necesaria para wait() y las macros de estado (WIFEXITED...)

/* * CRÍTICO PARA EL EXAMEN: Variable global para almacenar el PID del hijo.
 * Como el manejador asíncrono de señales 'manejador_SIGALRM' solo puede recibir 
 * el número de señal como argumento, no tiene parámetros para recibir variables locales.
 * La única forma de comunicarle el PID del hijo al manejador es mediante el espacio global.
 */
pid_t pid_hijo = 0; 

/* * FUNCIÓN MANEJADORA DE SIGALRM:
 * Se ejecutará de forma asíncrona única y exclusivamente si el reloj de alarm(5) llega a cero.
 */
void manejador_SIGALRM(int signo){
    (void)signo; // Evita el warning de compilación por parámetro no usado
    
    if (pid_hijo > 0) {
        printf("\n[MANEJADOR] ¡Tiempo límite de 5 segundos alcanzado! (SIGALRM recibido).\n");
        printf("[MANEJADOR] Enviando señal SIGKILL de forma fulminante al hijo (PID %d)...\n", pid_hijo);
        
        // Enviamos la señal 9 (SIGKILL), la cual el hijo no puede bloquear ni ignorar
        if(kill(pid_hijo, SIGKILL) == -1){
            perror("Error al ejecutar la llamada al sistema kill()");
        }
    }
}

/*
 * FUNCIÓN AUXILIAR: Configura el proceso para IGNORAR la señal SIGINT (Ctrl+C).
 */
void configurar_ignorar_sigint(){
    struct sigaction sa;
    
    sa.sa_handler = SIG_IGN;   // Asignamos la macro especial SIG_IGN (Ignore Signal)
    sa.sa_flags = 0;           // Comportamiento estándar sin banderas avanzadas
    sigemptyset(&sa.sa_mask);  // Inicializamos la máscara protectora totalmente vacía
    
    // Aplicamos la máscara a la señal de interrupción por teclado
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error al intentar ignorar SIGINT con sigaction");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    // Validación: El usuario debe indicar al menos un programa a ejecutar
    if(argc < 2){
        fprintf(stderr, "Error: Faltan argumentos.\nUso: %s <comando> [argumentos...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 1. REQUISITO: Configurar al proceso padre para ignorar SIGINT (Ctrl+C) */
    configurar_ignorar_sigint(); 
    printf("[PADRE] Configurado correctamente para ignorar SIGINT (Ctrl+C).\n");

    /* * 2. REQUISITO: Creación del único proceso hijo mediante fork()
     * ATENCIÓN: No uses funciones intermedias como launch_command si vas a hacer fork en main,
     * ya que provocarías un proceso "nieto" por accidente. Aquí se hace directamente de forma limpia.
     */
    pid_hijo = fork();
    
    if(pid_hijo < 0){
        perror("Error al realizar la llamada al sistema fork()");
        return EXIT_FAILURE;
    }
    
    if(pid_hijo == 0){
        /* ==================== CÓDIGO DEL PROCESO HIJO ==================== */
        printf("[HIJO] Mi PID es %d. Me dispongo a ejecutar: %s\n", getpid(), argv[1]);
        
        /* * Mutación del hijo mediante execvp:
         * - argv[1]: Nombre del binario a buscar en el PATH (ej: "ls" o "xeyes").
         * - &argv[1]: Subvector que actúa como el 'argv' del nuevo comando (contiene el comando y sus opciones).
         */
        execvp(argv[1], &argv[1]);   
        
        // Si el flujo de ejecución pisa esta línea, significa que execvp falló (ej: el comando no existe)
        perror("Error crítico al ejecutar execvp en el proceso hijo");
        exit(EXIT_FAILURE); 
    }
    else {  
        /* ==================== CÓDIGO DEL PROCESO PADRE ==================== */
        printf("[PADRE] Mi PID es %d. Mi hijo tiene el PID %d.\n", getpid(), pid_hijo);

        // 3. REQUISITO: Configurar la estructura sigaction para atrapar el temporizador
        struct sigaction sa_alarm;   
        sa_alarm.sa_handler = manejador_SIGALRM;  // Vinculamos nuestra función despertador
        sigemptyset(&sa_alarm.sa_mask);           // No bloqueamos ninguna otra señal durante el manejador
        sa_alarm.sa_flags = 0;                    // Comportamiento de interrupción estándar

        if(sigaction(SIGALRM, &sa_alarm, NULL) == -1){
            perror("Error al configurar sigaction para SIGALRM");
            exit(EXIT_FAILURE);
        }

        // 4. REQUISITO: Programar la cuenta atrás de 5 segundos en el Kernel
        printf("[PADRE] Activando el temporizador de 5 segundos...\n");
        alarm(5);

        int estado;
        printf("[PADRE] Entrando en estado de bloqueo (wait). Esperando al hijo...\n");
        
        // El proceso padre se duerme en esta línea hasta que el hijo termine o sea fulminado
        pid_t pid_espera = wait(&estado);

        if(pid_espera == -1){
            perror("Error en la llamada al sistema wait()");
            exit(EXIT_FAILURE);
        }

        /* * REQUISITO DE SEGURIDAD MÁXIMA: 
         * Si el proceso hijo fue rápido y terminó antes de los 5 segundos (ej: 'ls'),
         * debemos apagar el reloj haciendo alarm(0). Si no lo hiciéramos, la alarma sonaría
         * más tarde rompiendo el flujo normal del programa posterior.
         */
        alarm(0); 

        /* 5. REQUISITO: Evaluar y desglosar rigurosamente la causa del deceso del hijo */
        if(WIFEXITED(estado)){
            // El hijo finalizó de manera voluntaria (hizo un return en su main o llamó a exit())
            printf("[PADRE] El hijo (PID %d) ha terminado NORMALMENTE con el código de salida: %d.\n", 
                   pid_espera, WEXITSTATUS(estado));
        }
        else if(WIFSIGNALED(estado)){
            // El hijo colapsó por el impacto violento de una señal externa
            printf("[PADRE] El hijo (PID %d) fue TERMINADO VIOLENTAMENTE por la señal número: %d.\n", 
                   pid_espera, WTERMSIG(estado));
            if (WTERMSIG(estado) == SIGKILL) {
                printf("[PADRE] Confirmación: El hijo fue liquidado por el temporizador del programa (SIGKILL).\n");
            }
        }
    }

    printf("[PADRE] Programa finalizado limpiamente de forma correcta.\n");
    return 0;
}

/*
 * ====================================================================================
 * ANEXO TÉCNICO: EXPLICACIÓN DE LAS ALARMAS, FUNCIONES COMPLETAS Y FLUJO DE CONTROL
 * ====================================================================================
 *
 * 1. EL MECANISMO DE LAS ALARMAS (alarm) Y SEÑALES ASÍNCRONAS
 * ------------------------------------------------------------------------------------
 * Una alarma en sistemas UNIX/Linux es un temporizador de hardware gestionado de forma directa
 * por el Kernel a nivel de proceso. Cuando invocamos la llamada 'alarm(5)', el Kernel crea un 
 * contador regresivo en segundo plano. Nuestro programa principal puede seguir haciendo otras 
 * tareas o quedarse bloqueado esperando en un 'wait()'. Cuando el reloj llega a cero, el Kernel 
 * congela la ejecución del proceso de manera asíncrona y le inyecta la señal 'SIGALRM'.
 *
 * Por defecto, la recepción de un SIGALRM aborta y destruye el programa. Para evitarlo, se debe
 * registrar previamente un manejador ('sa_handler'). 
 *
 * FUNCIONES UTILIZADAS Y OPCIONES CONFIGURABLES (ALTERNATIVAS DE EXAMEN):
 * * - sigaction(): Diseña e inyecta la lógica de respuesta para una señal específica. Es la función 
 * estándar de POSIX (la función antigua 'signal()' está obsoleta).
 * > En 'sa_handler' podemos asignar tres opciones:
 * 1. El nombre de una función propia de C que procese la señal (ej: manejador_SIGALRM).
 * 2. SIG_IGN: Macro del sistema que ordena al Kernel ignorar y descartar la señal (la usamos para SIGINT).
 * 3. SIG_DFL: Restaura la acción destructiva o por defecto que tiene la señal en el sistema operativo.
 * > En 'sa_flags' pusimos '0' (estándar), pero se suelen evaluar opciones como:
 * 1. SA_RESTART: Fuerza a que las llamadas al sistema interrumpidas por la señal (como wait() o read())
 * se reanuden solas de forma automática tras salir del manejador en lugar de devolver -1.
 * 2. SA_RESETHAND: Hace que el manejador capture la señal una sola vez y luego se autodestruya.
 *
 * - kill(pid, señal): Envía una señal al identificador de proceso (PID) objetivo.
 * > Señales típicas: SIGKILL (número 9, muerte instantánea e imbloqueable), SIGTERM (petición 
 * educada de cierre), SIGSTOP (congela el proceso suspendiendo su ejecución).
 * > Alternativa: Si un proceso quiere enviarse una señal a sí mismo de forma directa, utiliza raise(señal).
 *
 * - wait(&estado) / waitpid(): Sincronizan al padre bloqueándolo en memoria hasta que el hijo muere.
 * Recogen la información de terminación en la variable entera entera. Se analiza obligatoriamente con macros:
 * > WIFEXITED: Verdadero si el proceso hijo finalizó por vía normal (return/exit).
 * > WEXITSTATUS: Extrae el número del código de salida devuelto por el hijo (ej. 0 si tuvo éxito).
 * > WIFSIGNALED: Verdadero si el proceso hijo colapsó debido al impacto de una señal.
 * > WTERMSIG: Extrae el número exacto de la señal que aniquiló al hijo (ej. 9 para SIGKILL).
 *
 * 2. DIAGRAMA Y EXPLICACIÓN DEL FLUJO DE EJECUCIÓN DEL CÓDIGO
 * ------------------------------------------------------------------------------------
 *//*
 * El procesador se bifurca y compite en el tiempo siguiendo dos flujos de ejecución excluyentes:
 *
 * PASO 1 (Blindaje): El programa principal ejecuta 'configurar_ignorar_sigint()'. Desde este momento,
 * si pulsas Ctrl+C en la consola, el Kernel envía un SIGINT pero el padre lo descarta, garantizando que 
 * el proceso monitorizador no muera prematuramente.
 *
 * PASO 2 (Bifurcación): Se ejecuta 'fork()'. El proceso se divide:
 * - El proceso HIJO entra en el bloque 'pid_hijo == 0', imprime su mensaje y ejecuta 'execvp()'. 
 * Su memoria se borra y muta al comando solicitado (por ejemplo: el programa infinito 'xeyes').
 * - El proceso PADRE almacena el PID real del hijo en la variable GLOBAL 'pid_hijo' y avanza al 'else'.
 *
 * PASO 3 (Temporización y Bloqueo): El padre configura su sigaction para asociar SIGALRM con su función, 
 * inicia el reloj con 'alarm(5)' y se queda durmiendo (bloqueado) en la llamada del sistema 'wait(&estado)'.
 *
 * A partir de aquí, la CPU resuelve la carrera siguiendo uno de los dos escenarios posibles:
 *
 * • ESCENARIO A (El comando asignado es RÁPIDO, ej: './signalsyfork ls'):
 * 1. El hijo termina su trabajo en milisegundos y muere limpiamente haciendo exit(0).
 * 2. El Kernel detecta la muerte del hijo y despierta de inmediato al padre que dormía en el wait().
 * 3. El padre avanza inmediatamente y ejecuta 'alarm(0)' apagando el despertador para que no suene tarde.
 * 4. Se evalúa 'WIFEXITED', da verdadero, e imprime en la consola el código de salida normal (0).
 * 5. El padre termina con éxito.
 *
 * • ESCENARIO B (El comando asignado es LENTO/INFINITO, ej: './signalsyfork xeyes' o 'sleep 20'):
 * 1. El hijo se queda colgado haciendo sus tareas y el padre sigue bloqueado durmiendo en el wait().
 * 2. Transcurren los 5 segundos de reloj y el Kernel dispara la alarma mandando un 'SIGALRM' al padre.
 * 3. El Kernel pausa el wait() del padre desvía el flujo de la CPU hacia 'manejador_SIGALRM()'.
 * 4. El manejador ejecuta 'kill(pid_hijo, SIGKILL)', destruyendo fulminantemente al hijo lento.
 * 5. El manejador termina y el flujo regresa al main. Al estar el hijo muerto, el wait() se destranca solo.
 * 6. Se evalúa 'WIFSIGNALED', da verdadero, e informa que el hijo murió por el impacto de la señal 9 (SIGKILL).
 * 7. El padre termina con éxito.
 */