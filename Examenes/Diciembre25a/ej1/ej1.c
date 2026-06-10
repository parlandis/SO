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
        //printf("Soy el proceso hijo :%d y voy a mutar\n", getpid());
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
    int B_mode = 0;   

        /*
     * Procesar opciones:
     *  -x "comando"  → Ejecuta un comando directo.
     *  -s fichero    → Ejecuta comandos línea a línea de un fichero.
     *  -b            → Si se usa con -s, lanza los comandos en background.
     */
    while((opt = getopt(argc, argv, "bx:s:B")) != -1){
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
        case 'B':   
            B_mode = 1;
            break;
            
        default:
            fprintf(stderr, "Opcion no reconocida\n");
            fprintf(stderr, "Usage: %s -x <\"command\"> | -s <fichero>  &| -b  \n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if(B_mode == 1){
        if(s_mode == 0){
            fprintf(stderr, "La opcion B necesita tener un archivo con la opcion s\n");
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

            if(B_mode == 1){

                pid_t pid;
                if(strcmp(cmd_argv[cmd_argc - 1], "&") == 0){
                    cmd_argv[cmd_argc -1] = NULL;
                    
                    printf("&\n");
                    printf("@@ Running Command #%d: %s\n",i,linea);
                     pid = launch_command(cmd_argv);

                    
                    pid_Array[comandI] = pid;
                    comandN[comandI] = comandI;
                    comandI++;
                    cont++;
                }
                else {
                    printf("@@ Running Command #%d: %s\n",i,linea);
                    pid_t pid = launch_command(cmd_argv);
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
            else {
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

        }

        




        if(b_mode == 1 ||B_mode == 1){
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
}