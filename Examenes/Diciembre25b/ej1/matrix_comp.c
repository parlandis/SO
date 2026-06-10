#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt, fork, execl */
#include <string.h>     /* strlen, strcspn, snprintf */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <ctype.h>
#include <sys/wait.h>   // waitpid, macros WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG

// Estructura obligatoria para el apartado d) para asociar el hijo con su fichero
typedef struct {
    pid_t pid;
    char *nombre_fichero;
} registro_t;

/*
 * matrix_compute
 * - Cambia el programa en ejecución reemplazando la imagen del hijo por la de Bash.
 * - Monta dinámicamente el pipeline: sed 's/ /<op>/g' <fichero> | bc > <fichero.out>
 * - Si execl falla, el hijo finaliza inmediatamente informando del error.
 */
void matrix_compute(const char *fichero_in, const char *fichero_out, char operacion) {
    char cmd[2048];

    // Montamos la cadena de formato exacta sugerida por el examen
    // Reemplaza los espacios por el operador, se lo manda a bc y lo redirige a .out
    snprintf(cmd, sizeof(cmd), "sed 's/ /%c/g' %s | bc > %s", operacion, fichero_in, fichero_out);

    // [EXAMEN] Mutamos al hijo delegando el comando compuesto en la Shell de Bash
    execl("/bin/bash", "bash", "-c", cmd, NULL);

    // Si llega aquí es que falló drásticamente el exec
    perror("Error en execl dentro de matrix_compute");
    exit(EXIT_FAILURE);
}

/*
 * main
 * - Procesa el flag -o con getopt verificando si es '+' o '*'.
 * - Recorre de forma asíncrona todos los ficheros de entrada mediante forks paralelos (Apartado b).
 * - Reserva memoria milimétrica para generar los nombres de salida con la extensión ".out" (Apartado a).
 * - Recoge todos los hijos informando detalladamente de su estado de finalización (Apartado d).
 */
int main(int argc, char *argv[]) {
    // Verificación de argumentos mínimos en la línea de comandos
    if (argc < 4) {
        fprintf(stderr, "Uso: %s -o \"+\"|\"*\" fichero1 [fichero2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }  

    int opt;
    char op = '\0';

    // [EXAMEN: CORREGIDO] getopt devuelve -1 al terminar, no 1
    while ((opt = getopt(argc, argv, "o:")) != -1) {
        switch (opt) {
        case 'o':
            op = optarg[0]; // Capturamos el primer carácter del argumento de la opción
            if (op != '+' && op != '*') {
                fprintf(stderr, "Error: La operación debe ser * o +\n");
                fprintf(stderr, "Uso: %s -o \"+\"|\"*\" fichero1 [fichero2 ...]\n", argv[0]);
                return EXIT_FAILURE;
            }
            break;
        default:
            fprintf(stderr, "Uso: %s -o \"+\"|\"*\" fichero1 [fichero2 ...]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Calculamos cuántos archivos nos han pasado para procesar
    int num_ficheros = argc - optind;
    if (num_ficheros < 1) {
        fprintf(stderr, "Error: Debe especificar al menos un archivo de entrada.\n");
        return EXIT_FAILURE;
    }

    // Array de estructuras para registrar la telemetría de recogida de los hijos
    registro_t tabla_hijos[num_ficheros];
    int cont = 0; // Contador de procesos hijos lanzados con éxito

    // =========================================================================
    // APARTADOS A y B: BUCLE DE LANZAMIENTO PARALELO (LOS HIJOS NO ESPERAN)
    // =========================================================================
    for (int i = optind; i < argc; i++) {
        char *in = argv[i];

        // i) [EXAMEN] Generar el nombre de salida dinámico añadiendo ".out"
        // Tamaño: longitud de 'in' + 4 bytes de ".out" + 1 byte para '\0'
        char *out = malloc(strlen(in) + 5);
        if (out == NULL) {
            perror("malloc out");
            return EXIT_FAILURE;
        }
        snprintf(out, strlen(in) + 5, "%s.out", in);

        // ii) [EXAMEN] Lanzar proceso hijo en paralelo
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error al realizar el fork");
            free(out);
            return EXIT_FAILURE;
        } 
        else if (pid == 0) {
            // --- CÓDIGO DEL PROCESO HIJO ---
            matrix_compute(in, out, op);
        } 
        else {
            // --- CÓDIGO DEL PROCESO PADRE ---
            // Registramos los datos del hijo para identificarlos en la recogida asíncrona
            tabla_hijos[cont].pid = pid;
            tabla_hijos[cont].nombre_fichero = in; // Guardamos el nombre original
            cont++;
            
            // Liberamos la memoria del string out en el padre (el hijo ya mutó y no la necesita)
            free(out); 
        }
    }

    // =========================================================================
    // APARTADO D: RECOGIDA ASÍNCRONA DE TODOS LOS PROCESOS HIJOS
    // =========================================================================
    int status;
    
    // Hacemos exactamente tantos waitpid como hijos pusimos en marcha en el bucle anterior
    for (int i = 0; i < cont; i++) {
        // waitpid con -1 y opción 0 duerme al padre de manera eficiente hasta que CUALQUIER hijo muera
        pid_t pid_terminado = waitpid(-1, &status, 0);

        if (pid_terminado > 0) {
            char *fichero_asociado = "Desconocido";

            // Buscamos a qué archivo de texto pertenecía el PID que acaba de despertar al padre
            for (int j = 0; j < cont; j++) {
                if (tabla_hijos[j].pid == pid_terminado) {
                    fichero_asociado = tabla_hijos[j].nombre_fichero;
                    break;
                }
            }

            // [EXAMEN: TRATAMIENTO DE MACROS OBLIGATORIO]
            if (WIFEXITED(status)) {
                // El proceso terminó de manera natural por un exit() o return
                printf("[%d] operating on %s exited with status %d\n", 
                       pid_terminado, fichero_asociado, WEXITSTATUS(status));
            } 
            else if (WIFSIGNALED(status)) {
                // El proceso fue fulminado de forma violenta por una señal del sistema
                printf("[%d] operating on %s terminated by signal %d\n", 
                       pid_terminado, fichero_asociado, WTERMSIG(status));
            }
        }
    }

    return EXIT_SUCCESS;
}