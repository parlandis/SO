#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE, malloc, free */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt, execl, getpid */
#include <string.h>     /* strlen, strcmp, strcpy */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <ctype.h>
#include <sys/wait.h>   /* waitpid, macros WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG */

// Estructura para el apartado d): Asociar cada PID con el nombre del fichero que procesa
typedef struct {
    pid_t pid;
    char *nombre_fichero;
} registro_proceso_t;

/**
 * Apartado c)
 * Cambia el ejecutable actual por una instancia de bash para ejecutar la tubería:
 * sed 's/ /<op>/g' <fichero_in> | bc > <fichero_out>
 */
void matrix_compute(char *fichero_in, char *fichero_out, char *op) {
    // Cadena de formato recomendada en la plantilla del enunciado
// Esta versión elimina los espacios al final de la línea (\s*$) y los \r antes de poner los '+' o '*'
#define CMD_FORMAT "sed 's/\\r//g; s/[[:space:]]*$//g; s/ /%c/g' %s | bc > %s"
    
    char cmd[512];
    char operacion_char = op[0]; // Extraemos el carácter '+' o '*' de la cadena

    // Construimos el comando final sustituyendo los valores en la plantilla
    snprintf(cmd, sizeof(cmd), CMD_FORMAT, operacion_char, fichero_in, fichero_out);

    // Reemplazamos el proceso hijo por la shell bash pasándole el comando preparado
    execl("/bin/bash", "bash", "-c", cmd, NULL);

    // Si execl regresa, significa que se produjo un error grave
    perror("Error crítico al ejecutar execl");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    // Validación inicial de parámetros mínimos de entrada
    if (argc < 3) {
        fprintf(stderr, "Uso: %s -o <+|+> <fichero1> [<fichero2> ...]\n", argv[0]);
        return EXIT_FAILURE;
    }  

    int opt;
    char *op = NULL;

    // Apartado a): Procesamiento del flag -o mediante getopt
    // Los dos puntos ":" tras la 'o' indican que este flag requiere un argumento obligatorio
    while ((opt = getopt(argc, argv, "o:")) != -1) {
        switch (opt) {
        case 'o':
            op = optarg; // Guardamos el argumento capturado por getopt (+ o *)
            if ((strcmp(op, "+") != 0) && (strcmp(op, "*") != 0)) {
                fprintf(stderr, "Error: La operación debe ser '*' o '+'\n");
                return EXIT_FAILURE;
            }
            break;
        default: 
            fprintf(stderr, "Uso: %s -o <+|+> <fichero1> [<fichero2> ...]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Validación de que existan ficheros tras las opciones de la línea de comandos
    if (argc - optind < 1) {
        fprintf(stderr, "Error: Debe especificar al menos un archivo de entrada.\n");
        return EXIT_FAILURE;
    }

    int num_ficheros = argc - optind;
    registro_proceso_t tabla_procesos[num_ficheros]; // Array para guardar qué procesa cada PID
    int cont = 0;

    // Apartado b): Lanzamiento de procesos en paralelo para cada fichero
    for (int j = optind; j < argc; j++) {
        char *in = argv[j];

        // i) Generar el nombre dinámico del fichero de salida (.out)
        // Reservamos espacio para la longitud del fichero + 4 caracteres de ".out" + 1 para '\0'
        char *out = malloc(strlen(in) + 5); 
        if (out == NULL) {
            perror("Error en malloc al reservar memoria para el nombre de salida");
            return EXIT_FAILURE;
        }
        snprintf(out, strlen(in) + 5, "%s.out", in);

        // Realizamos la llamada a fork para duplicar el proceso
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Error al realizar el fork");
            free(out);
            return EXIT_FAILURE;
        }
        else if (pid == 0) {
            // --- Código del Proceso Hijo ---
            // ii) Invocar la función matrix_compute pasándole los parámetros
            matrix_compute(in, out, op);
        }
        else {
            // --- Código del Proceso Padre ---
            // Registramos el PID del hijo y asociamos su nombre de fichero de entrada
            tabla_procesos[cont].pid = pid;
            tabla_procesos[cont].nombre_fichero = in;
            cont++;
        }

        // Liberamos la cadena out en el padre (el hijo ya mutó con exec)
        free(out);
    }

    printf("[PADRE PID: %d] Todos los hijos han sido lanzados. Esperando terminación...\n", getpid());
    
    // Apartado d): Esperar de forma asíncrona a que terminen todos los hijos lanzados
    int status; 
    for (int i = 0; i < cont; i++) {
        // waitpid(-1, ...) espera a cualquier hijo que finalice sin importar el orden
        pid_t terminado = waitpid(-1, &status, 0);

        if (terminado > 0) {
            // Buscamos en nuestra tabla de registros a qué fichero pertenecía este PID terminado
            char *fichero_procesado = "Desconocido";
            for (int k = 0; k < cont; k++) {
                if (tabla_procesos[k].pid == terminado) {
                    fichero_procesado = tabla_procesos[k].nombre_fichero;
                    break;
                }
            }

            // Mostramos la información de finalización requerida por el enunciado
            if (WIFEXITED(status)) {
                int codigo_retorno = WEXITSTATUS(status);
                printf("[%d] operating on %s exited with status %d\n", 
                       terminado, fichero_procesado, codigo_retorno);
            } 
            else if (WIFSIGNALED(status)) {
                int senal = WTERMSIG(status);
                printf("[%d] operating on %s terminated by signal %d\n", 
                       terminado, fichero_procesado, senal);
            }
        }
    }

    printf("[PADRE PID: %d] Todos los hijos han terminado con éxito. Fin del programa principal.\n", getpid());
    return EXIT_SUCCESS;
}