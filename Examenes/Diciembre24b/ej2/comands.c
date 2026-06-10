#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt, fork, execvp, execl */
#include <string.h>     /* strlen, strcspn */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <ctype.h>
#include <sys/wait.h>   // waitpid, macros WEXITSTATUS, WNOHANG

/*
 * launch_command
 * - Recibe un vector de argumentos argv (del comando a ejecutar).
 * - Crea un proceso hijo mediante fork().
 * - El proceso hijo muta su imagen ejecutando el comando clásico con execvp.
 * - Devuelve el PID del hijo al proceso padre.
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
 * - Recibe una cadena cmd, por ejemplo: "ls -l /tmp".
 * - La separa por espacios en argumentos: ["ls", "-l", "/tmp", NULL].
 * - Reserva memoria dinámicamente para el vector de punteros (argv) y para cada string.
 * - Devuelve el vector argv y deja el número de argumentos en *argc.
 */
char **parse_command(const char *cmd, int* argc) {
    size_t argv_size = 10;       // Tamaño inicial del array de punteros
    const char *end;
    size_t arg_len; 
    int arg_count = 0;           // Número de argumentos detectados
    const char *start = cmd;     // Puntero de recorrido sobre la cadena de entrada
    char **argv = malloc(argv_size * sizeof(char *));

    printf("%s\n", cmd);

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

/*
 * launch_shell_command
 * - Recibe una cadena de texto plana cmd con el comando completo.
 * - Crea un proceso hijo mediante fork().
 * - El hijo muta llamando a /bin/bash con la opción "-c" para tragar pipes, redirecciones y variables.
 * - Devuelve el PID del proceso hijo al padre.
 */
pid_t launch_shell_command(char* cmd){
    pid_t pid;

    pid = fork();
    if(pid < 0){
        perror("Error en el proceso hijo");
        exit(EXIT_FAILURE);
    } else if(pid == 0){
        printf("Soy el proceso hijo :%d y voy a mutar BASH\n", getpid());

        // [EXAMEN] Se pasa la cadena entera cmd sin parsear, Bash se encarga de todo
        execl("/bin/bash", "bash", "-c", cmd, NULL);
        
        perror("Execl");
        exit(EXIT_FAILURE);
    }
    return pid;
}

/*
 * main
 * - Punto de entrada principal del programa run_commands.
 * - Procesa opciones con getopt: -x (directo), -s (fichero script), -b (background), -i (interactivo), -B (bash interactivo).
 * - Gestiona los flujos e invoca el parseador y lanzador correspondiente según el modo activo.
 */
int main(int argc, char *argv[]) {
    char **cmd_argv;      // Vector de argumentos de cada comando parseado
    int cmd_argc;          // Número de argumentos del comando parseado

    if (argc < 2) {
        fprintf(stderr, "Usage: %s -x <\"command\"> | -s <fichero>  &| -b  \n", argv[0]);
        return EXIT_FAILURE;
    }

    int opt;
    int i_mode = 0;
    int B_mode = 0;

    // Procesar las opciones con getopt
    while((opt = getopt(argc, argv, "Bx:i")) != -1){
        switch (opt)
        {
        case 'x':
            printf("Ejecutando comando: %s\n", optarg);
            cmd_argv = parse_command(optarg, &cmd_argc);
            pid_t pid =  launch_command(cmd_argv);
            int status;
            waitpid(pid, &status, 0);
            printf("Command terminated (pid: %d, status: %d)\n",pid, WEXITSTATUS(status));

            // Liberar memoria de los argumentos
            for (int i = 0; i < cmd_argc; i++) {
                free(cmd_argv[i]);
            }
            free(cmd_argv);
            break;
        case 'i':
            i_mode = 1;
            break;
        case 'B':
            B_mode = 1;
            break;
        default:
            fprintf(stderr, "Opcion no reconocida\n");
            fprintf(stderr, "Usage: %s -x <\"command\"> | -s <fichero>  &| -b  \n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // =========================================================================
    // MODOS -i / -B: MODO INTERACTIVO (PREGUNTADO EN EL ENUNCIADO)
    // =========================================================================
    if(i_mode == 1){
        char linea[256];

        // Pintamos el prompt inicial exigido por el enunciado
        printf("> ");
        fflush(stdout); // [EXAMEN] Obligatorio para asegurar que el prompt sale en Ubuntu al instante

        // Bucle interactivo controlado por fgets hasta detectar fin de fichero (CTRL+D -> NULL)
        while(fgets(linea, sizeof(linea), stdin) != NULL){
            
            // [EXAMEN] Eliminamos de forma segura el salto de línea (\n y \r) con strcspn
            linea[strcspn(linea, "\r\n")] = '\0';
            
            // Si el usuario introduce una línea vacía (solo pulsa enter), pasamos olímpicamente
            if (strlen(linea) == 0) {
                printf("> ");
                fflush(stdout);
                continue;
            }

            printf("Comando a ejecutar: %s\n", linea);

            // Si NO estamos en modo Bash (-B), el padre tiene que trocear el comando
            if(B_mode == 0){
                cmd_argv = parse_command(linea, &cmd_argc);
            }
            
            pid_t pid;
            printf("@@ Running Command: %s\n", linea);
            
            // Bifurcación de lanzamiento según los flags activos
            if(B_mode == 1){
                pid = launch_shell_command(linea); // Llama a Bash puro
            }
            else {
                pid = launch_command(cmd_argv);   // Llama a execvp clásico
            }

            // Sincronización en primer plano: El padre espera a que el hijo acabe
            int status;
            if (pid > 0) {
                waitpid(pid, &status, 0);
                printf("Commando teminado (pid: %d, status: %d)\n\n", pid, WEXITSTATUS(status));
            }

            // [EXAMEN: CRÍTICO] Liberamos la memoria dinámica del Heap únicamente si usamos parse_command
            if(B_mode == 0){
                for(int k = 0; k < cmd_argc; k++) {
                    free(cmd_argv[k]);
                }
                free(cmd_argv);
            }

            // Pintamos de nuevo el prompt para la siguiente orden interactiva
            printf("> ");
            fflush(stdout);   
        }
    }

    return 0;
}