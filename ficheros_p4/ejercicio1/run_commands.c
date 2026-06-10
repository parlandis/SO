#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt */
#include <string.h>     /* strlen */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <ctype.h>
#include <sys/wait.h>   // waitpid, macros WEXITSTATUS, WNOHANG




/*
 * launch_command
 *  - Recibe un vector de argumentos al estilo execvp: argv[0] = comando, argv[1..] = args, argv[n] = NULL.
 *  - Crea un proceso hijo con fork.
 *  - En el hijo, llama a execvp(argv[0], argv) para "mutar" y ejecutar el comando.
 *  - En el padre, devuelve el PID del hijo para poder hacer waitpid desde fuera.
 */
pid_t launch_command(char** argv){
    pid_t pid;

    pid = fork();
    if(pid < 0){
        perror("Error en el proceso hijo");
        exit(EXIT_FAILURE);
    } else if(pid == 0){
        printf("Soy el proceso hijo :%d y voy a mutar\n", getpid());
         // Reemplazamos la imagen del proceso por el comando solicitado
        execvp(argv[0], argv);
        perror("Execvp");
        exit(EXIT_FAILURE);
    }
    return pid;
}


/*
 * parse_command
 *  - Recibe una cadena cmd, por ejemplo: "ls -l /tmp".
 *  - La separa por espacios en argumentos: ["ls", "-l", "/tmp", NULL].
 *  - Reserva memoria dinámicamente para el vector de punteros (argv) y para cada string.
 *  - Devuelve el vector argv y deja el número de argumentos en *argc.
 */
char **parse_command(const char *cmd, int* argc) {
    size_t argv_size = 10;       // Tamaño inicial del array de punteros
    const char *end;
    size_t arg_len; 
    int arg_count = 0;           // Número de argumentos detectados
    const char *start = cmd;     // Puntero de recorrido sobre la cadena de entrada
    char **argv = malloc(argv_size * sizeof(char *));

    if (argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while (*start && isspace(*start)) start++; // Saltar espacios iniciales

    // Bucle principal de tokenización
    while(*start){
         // Si no queda espacio en argv, lo ampliamos con realloc
        if(arg_count > argv_size -1){  // Reservar hueco para el NULL final
            argv_size *= 2;

            argv = realloc(argv, argv_size * sizeof(char *));
            if(argv == NULL){
                perror("Realloc");
                exit(EXIT_FAILURE);
            }
        }
            // 'end' avanza desde start hasta el siguiente espacio o fin de cadena
            end = start;
            while(*end && !isspace(*end)) end++;

            // Reservar espacio para el argumento (longitud + 1 para '\0')
            arg_len = end - start;
            argv[arg_count] = malloc(arg_len + 1);

            if (argv[arg_count] == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            // Copiar el trozo de texto y añadir '\0'
            strncpy(argv[arg_count], start, arg_len);
            argv[arg_count][arg_len] = '\0';
            arg_count++;

            // Avanzar al siguiente token, saltando espacios
            start = end;
            while (*start && isspace(*start)) start++;
            
    }
    argv[arg_count] = NULL;// Terminar el array de punteros con NULL, como exige execvp

    (*argc)=arg_count; // Devolver el número de argumentos al llamador
    
    return argv;
}


int main(int argc, char *argv[]) {
    char **cmd_argv;      // Vector de argumentos de cada comando parseado
    int cmd_argc;          // Número de argumentos del comando parseado
    int i;
    char * fichero;

    pid_t pid_Array[10];    // Array de PIDs de procesos hijos (para modo background)
    int comandN[10];        // Asocia un índice de comando con un PID concreto
    int cont = 0;           // Número total de comandos lanzados en background
    int comandI = 0;        // Índice para rellenar pid_Array y comandN

    if (argc <3 ) {
        fprintf(stderr, "Usage: %s -x <\"command\"> | -s <fichero>  &| -b  \n", argv[0]);
        return EXIT_FAILURE;
    }

    int opt;
    int b_mode = 0;    // Flags: s → usar script de fichero; b → modo background
    int s_mode = 0;

        /*
     * Procesar opciones:
     *  -x "comando"  → Ejecuta un comando directo.
     *  -s fichero    → Ejecuta comandos línea a línea de un fichero.
     *  -b            → Si se usa con -s, lanza los comandos en background.
     */
    while((opt = getopt(argc, argv, "bx:s:")) != -1){
        switch (opt)
        {
        case 'b':
            b_mode = 1;
            printf("B mode activado\n");
            break;
        case 'x':
            printf("Ejecutando comando: %s\n", optarg);
            // optarg contiene la cadena con el comando, por ejemplo "ls -l"
            cmd_argv = parse_command(optarg, &cmd_argc);
            pid_t pid =  launch_command(cmd_argv);
            int status;
            // Esperar a que ese proceso hijo termine (modo sincrónico)
            waitpid(pid, &status, 0);
            printf("Command terminated (pid: %d, status: %d)\n",pid, WEXITSTATUS(status));

            // Liberar memoria de los argumentos
            for (int i = 0; i < cmd_argc; i++) {
                free(cmd_argv[i]);
            }
            free(cmd_argv);
            break;
        case 's':
            s_mode = 1;
            fichero = optarg;
            printf("Fichero de comando a ejecutar: %s\n", fichero);
            break;
        default:
            fprintf(stderr, "Opcion no reconocida\n");
            fprintf(stderr, "Usage: %s -x <\"command\"> | -s <fichero>  &| -b  \n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    FILE *fp;
    if(s_mode == 1){
        if((fp = fopen(fichero, "r")) == NULL){
            fprintf(stderr, "Error al abrir el archivo\n");
            return EXIT_FAILURE;
        }

        //Vamos a leer el archivo por linea y sacamos el comando 
        char linea[256];
        i = 0;

        while(fgets(linea, sizeof(linea), fp) != NULL){
            if(linea[strlen(linea) - 1] == '\n'){
                linea[strlen(linea) - 1] = '\0';
            }   
            
           // printf("Hemos limpiado la linea\n Se va a ejecutar: %s\n", linea);

            cmd_argv = parse_command(linea, &cmd_argc);
            if(cmd_argc == 0 ||cmd_argv[0] == NULL){  // Si la linea es vacia la mandara vacia t volvera como NULL 
                free(cmd_argv);
                continue;
            }
            printf("@@ Running Command #%d: %s\n",i,linea);
            pid_t pid = launch_command(cmd_argv);

            if(b_mode == 0){
                int status;
                waitpid(pid, &status, 0);
                printf("Commando teminado (pid: %d, status: %d)\n\n ", pid, WEXITSTATUS(status));
            }
            else {

                pid_Array[comandI] = pid;
                comandN[comandI] = comandI;
                comandI++;
                cont++;

            }

                        // Liberar memoria de los argumentos de la línea actual
            for (int i = 0; i < cmd_argc; i++) {
                free(cmd_argv[i]);
            }
            free(cmd_argv);
            i++;

        }

        if(b_mode == 1){
              int status;
                pid_t pid;

                int completo = 0;
                while(completo < cont){
                // Esperar a cualquier hijo (-1), pero sin bloquear (WNOHANG)
                pid = waitpid(-1, &status, WNOHANG);

                if(pid > 0){
                    // Buscar qué comando corresponde a ese PID
                    for(int i = 0; i < cont; i++){
                        if(pid_Array[i] == pid){
                            printf("@@ Command #%d terminated (pid: %d, status: %d)\n",comandN[i], pid, WEXITSTATUS(status));
                            completo++;
                            break;
                        }
                    }
                }
                else if (pid == 0) {
                    // Ningún proceso ha terminado todavía; dormir un poco
                    usleep(10000); // 10 ms para no saturar la CPU
                }
                // Si pid < 0 habría que tratar errores, pero simplificamos aquí
                 }
        }


        


    }




    /*
    cmd_argv=parse_command(argv[1],&cmd_argc);

    // Print parsed arguments
    printf("argc: %d\n", cmd_argc);
    for (i = 0; cmd_argv[i] != NULL; i++) {
        printf("argv[%d]: %s\n", i, cmd_argv[i]);
        free(cmd_argv[i]);  // Free individual argument
    }
   */

 

    return EXIT_SUCCESS;
}


/*
 * FUNCIONES CLAVE EXPLICADAS:
 *
 * fork(void)
 *  - Crea un nuevo proceso (hijo) duplicando el proceso actual.
 *  - Devuelve:
 *      > 0 : al padre, el valor es el PID del hijo.
 *      = 0 : al hijo.
 *      < 0 : si ocurre un error y no se crea el hijo.
 *  - En este programa se usa en launch_command() para que el hijo pueda ejecutar
 *    el comando y el padre seguir controlándolo (esperando con waitpid, etc.).
 *
 * execvp(const char *file, char *const argv[])
 *  - Sustituye el código del proceso actual por el programa indicado en 'file',
 *    usando la lista de argumentos 'argv' (terminada en NULL).
 *  - Busca 'file' en el PATH igual que hace la shell (por ejemplo "ls", "pwd"...).
 *  - Solo vuelve en caso de error; si tiene éxito, el proceso ya es el nuevo programa.
 *  - Aquí se usa en el hijo para “mutar” y ejecutar realmente el comando pedido.
 * 
 *  l (List / Lista): Los argumentos del nuevo programa se pasan uno a uno como parámetros de la función, separados por comas.
 *  El último argumento tiene que ser obligatoriamente NULL.
    v (Vector): Los argumentos se pasan agrupados dentro de un array de strings (un vector de punteros char *[]), 
    de forma idéntica a cómo los recibe el main (argv). El último elemento del array debe ser NULL.
    p (Path / Ruta): No hace falta que le des la ruta absoluta del ejecutable (como /bin/ls).
     Si le pasas solo el nombre del comando ("ls"), la función lo buscará automáticamente en las carpetas del sistema 
     configuradas en la variable de entorno PATH (como /bin, /usr/bin, etc.).
    e (Environment / Entorno): Permite pasarle al nuevo programa un array personalizado con sus propias variables de entorno. 
    Si la función no lleva la e, el nuevo programa hereda las variables de entorno del proceso padre.
 *
 * waitpid(pid_t pid, int *status, int options)
 *  - Espera a que termine un proceso hijo.
 *  - pid:
 *      > 0 : espera a un hijo concreto con ese PID.
 *      = -1: espera a cualquier hijo.
 *  - options:
 *      0        → llamada bloqueante (se queda esperando).
 *      WNOHANG  → no bloquea; si ningún hijo ha terminado, devuelve 0.
 *  - 'status' guarda información sobre cómo terminó el hijo; se usa con:
 *      WEXITSTATUS(status) → devuelve el código de salida del hijo (return de main).
 *  - En este programa:
 *      - Sin -b se usa waitpid(pid, &status, 0) para esperar a cada comando.
 *      - Con -b se usa waitpid(-1, &status, WNOHANG) para ir recogiendo hijos
 *        que terminan en segundo plano sin bloquear la ejecución.
 *
 * getopt(int argc, char * const argv[], const char *optstring)
 *  - Analiza las opciones cortas de la línea de comandos (por ejemplo -x, -s, -b).
 *  - En cada llamada devuelve:
 *      'x', 's', 'b', ... → la opción encontrada.
 *      -1                 → cuando no quedan más opciones.
 *  - Variables asociadas:
 *      optarg → apunta al argumento de la opción actual (por ejemplo, la cadena
 *               de comando que sigue a -x o el nombre de fichero que sigue a -s).
 *      optind → índice en argv del siguiente elemento a procesar.
 *  - En este programa se usa para activar:
 *      -x "cmd"  → ejecutar un comando directo,
 *      -s file   → leer y ejecutar comandos desde un fichero,
 *      -b        → ejecutar los comandos del fichero en background.
 *
 * malloc(size_t size), realloc(void *ptr, size_t new_size), free(void *ptr)
 *  - malloc reserva 'size' bytes de memoria dinámica y devuelve un puntero.
 *  - realloc cambia el tamaño del bloque apuntado por 'ptr' conservando el contenido.
 *  - free libera memoria previamente reservada, evitando fugas.
 *  - En parse_command():
 *      - Se usa malloc para crear el array de punteros argv y cada argumento (string).
 *      - Se usa realloc para ampliar argv si hay más argumentos de los previstos.
 *      - Al terminar de usar argv, se libera cada argv[i] y luego el propio argv.
 *
 * fgets(char *s, int size, FILE *stream)
 *  - Lee una línea de texto de 'stream' en el buffer 's', como máximo size-1
 *    caracteres, y añade '\0' al final.
 *  - Incluye el '\n' si cabe; si se quiere quitar, hay que hacerlo manualmente.
 *  - Devuelve s si lee algo, o NULL en EOF o error.
 *  - En este programa se usa para leer cada línea del fichero de comandos en el modo -s.
 *
 * usleep(useconds_t usec)
 *  - Suspende el proceso durante 'usec' microsegundos.
 *  - Aquí se utiliza en el bucle con waitpid(-1, ..., WNOHANG) para esperar
 *    un poco cuando ningún hijo ha terminado aún, evitando usar el 100% de CPU.
 *
 * perror(const char *msg)
 *  - Imprime el mensaje 'msg' seguido de ": " y el texto asociado al último error
 *    de sistema (errno).
 *  - Se usa después de fallos en fork, execvp, fopen, malloc, realloc, etc.,
 *    para informar claramente de la causa del error.
 *
 * En conjunto, el programa combina:
 *  - Creación de procesos (fork),
 *  - Carga y ejecución de comandos externos (execvp),
 *  - Espera y control de la terminación de procesos hijo (waitpid),
 *  - Análisis de opciones de la línea de comandos (getopt),
 *  - Gestión dinámica de memoria para construir argv[] (malloc/realloc/free),
 *  - Lectura de scripts de comandos desde fichero (fgets),
 *  para implementar una pequeña shell que puede ejecutar comandos individuales,
 *  leer listas de comandos desde un archivo y lanzarlos en primer plano o en
 *  background.
 */