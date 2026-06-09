#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>     /* fork, execvp, getopt */
#include <sys/wait.h>   /* wait, waitpid, macros WIFEXITED, WEXITSTATUS */

#define MAX_ARGS 64
#define MAX_LINE 1024

// Estructura para el apartado -b: asociar el PID con el orden del comando
typedef struct {
    pid_t pid;
    int num_comando;
} proceso_t;

/**
 * Función requerida por el enunciado.
 * Se encarga de hacer fork() y mutar al hijo con execvp().
 * Devuelve el PID del hijo inmediatamente sin esperar.
 */
pid_t launch_command(char** argv) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        return -1;
    } else if (pid == 0) {
        // --- Código del Proceso Hijo ---
        execvp(argv[0], argv);
        
        // Si execvp retorna, es que el comando no se pudo ejecutar
        perror("Error en execvp");
        exit(EXIT_FAILURE);
    }
    
    // --- Código del Proceso Padre ---
    return pid;
}

/**
 * Función proporcionada en la plantilla original.
 * Trocea una cadena y devuelve un array en formato argv acabado en NULL.
 */
char **parse_command(const char *cmd, int* argc) {
    size_t argv_size = 10;
    const char *end;
    size_t arg_len; 
    int arg_count = 0;
    const char *start = cmd;
    char **argv = malloc(argv_size * sizeof(char *));

    if (argv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while (*start && isspace(*start)) start++; 

    while (*start) {
        if (arg_count >= argv_size - 1) {  
            argv_size *= 2;
            argv = realloc(argv, argv_size * sizeof(char *));
            if (argv == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }

        end = start;
        while (*end && !isspace(*end)) end++;

        arg_len = end - start;
        argv[arg_count] = malloc(arg_len + 1);

        if (argv[arg_count] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strncpy(argv[arg_count], start, arg_len);
        argv[arg_count][arg_len] = '\0';  
        arg_count++;

        start = end;
        while (*start && isspace(*start)) start++;
    }

    argv[arg_count] = NULL; 
    (*argc) = arg_count; 

    return argv;
}

/**
 * Función auxiliar para liberar la memoria de un array estilo argv
 */
void free_cmd_argv(char** argv) {
    if (argv == NULL) return;
    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
    }
    free(argv);
}

int main(int argc, char *argv[]) {
    int opt;
    char *cmd_arg = NULL;
    char *file_arg = NULL;
    int background_flag = 0; // Almacena si se ha introducido la opción -b

    // Procesamos las opciones de la línea de comandos mediante getopt()
    while ((opt = getopt(argc, argv, "x:s:b")) != -1) {
        switch (opt) {
        case 'x':
            cmd_arg = optarg;
            break;
        case 's':
            file_arg = optarg;
            break;
        case 'b':
            background_flag = 1;
            break;
        default:
            fprintf(stderr, "Uso: %s [-x <comando>] [-s <fichero> [-b]]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // =========================================================================
    // CASO 1: Opción -x <comando>
    // =========================================================================
    if (cmd_arg != NULL) {
        int cmd_argc;
        // Parseamos el comando con la función de la práctica
        char **cmd_argv = parse_command(cmd_arg, &cmd_argc);
        
        if (cmd_argv[0] != NULL) {
            pid_t pid = launch_command(cmd_argv);
            if (pid > 0) {
                int status;
                // El padre espera estrictamente a que termine este hijo único
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    printf("@@ Command terminated (pid: %d, status: %d)\n", pid, WEXITSTATUS(status));
                }
            }
        }
        free_cmd_argv(cmd_argv);
    }

    // =========================================================================
    // CASO 2: Opción -s <fichero> [-b]
    // =========================================================================
    if (file_arg != NULL) {
        FILE *f = fopen(file_arg, "r");
        if (f == NULL) {
            perror("Error al abrir el fichero de comandos");
            return EXIT_FAILURE;
        }

        char linea[MAX_LINE];
        int num_com = 0;

        // Estructura para registrar los procesos en background (asíncronos)
        proceso_t hijos_bg[256]; 
        int total_hijos_bg = 0;

        // Leemos el fichero línea por línea usando fgets()
        while (fgets(linea, sizeof(linea), f) != NULL) {
            // Ignorar líneas vacías, saltos de línea residuales o comentarios '#'
            if (linea[0] == '\n' || linea[0] == '\r' || linea[0] == '#') continue;

            // Limpiamos el salto de línea al final para poder imprimir de forma limpia
            linea[strcspn(linea, "\r\n")] = '\0';

            int cmd_argc;
            char **cmd_parsed = parse_command(linea, &cmd_argc);
            
            // Si la línea no contenía comandos válidos, saltamos
            if (cmd_parsed[0] == NULL) {
                free_cmd_argv(cmd_parsed);
                continue;
            }

            printf("@@ Running command #%d: %s\n", num_com, linea);

            // Lanzamos el comando
            pid_t pid = launch_command(cmd_parsed);

            if (pid > 0) {
                if (background_flag) {
                    // Si se activa -b, guardamos los datos del hijo para esperarlo al final
                    hijos_bg[total_hijos_bg].pid = pid;
                    hijos_bg[total_hijos_bg].num_comando = num_com;
                    total_hijos_bg++;
                } else {
                    // Si NO hay -b, ejecución secuencial: esperamos inmediatamente
                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) {
                        printf("@@ Command #%d terminated (pid: %d, status: %d)\n", num_com, pid, WEXITSTATUS(status));
                    }
                }
            }

            free_cmd_argv(cmd_parsed);
            num_com++;
        }
        fclose(f);

        // Si se usó la opción -b, esperamos de forma asíncrona a todos los hijos creados
        if (background_flag) {
            int hijos_restantes = total_hijos_bg;
            while (hijos_restantes > 0) {
                int status;
                // wait(&status) suspende al padre hasta que muera CUALQUIER proceso hijo
                pid_t pid_terminado = wait(&status); 
                
                if (pid_terminado > 0) {
                    // Buscamos a qué número de comando guardado correspondía este PID
                    int num_asociado = -1;
                    for (int i = 0; i < total_hijos_bg; i++) {
                        if (hijos_bg[i].pid == pid_terminado) {
                            num_asociado = hijos_bg[i].num_comando;
                            break;
                        }
                    }
                    if (WIFEXITED(status)) {
                        printf("@@ Command #%d terminated (pid: %d, status: %d)\n", num_asociado, pid_terminado, WEXITSTATUS(status));
                    }
                    hijos_restantes--;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}