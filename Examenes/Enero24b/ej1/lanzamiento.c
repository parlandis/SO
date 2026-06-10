#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt, execl, fork */
#include <string.h>     /* strlen, strcspn */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <ctype.h>
#include <sys/wait.h>   /* waitpid, macros WIFEXITED, WEXITSTATUS */

#define MAX_COMANDOS 256

// [EXAMEN: OBLIGATORIO APARTADO C] Estructura de datos para asociar el PID con el número de orden
typedef struct {
    pid_t pid;
    int num_comando;
} registro_t;

/**
 * Apartado a) - Función encargada de lanzar el proceso hijo mutando a bash
 */
pid_t run_command(const char *command) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Error crítico en fork");
        return -1;
    }
    else if (pid == 0) {
        // --- CÓDIGO DEL PROCESO HIJO ---
        // Usamos bash -c para admitir cualquier comando compuesto
        execl("/bin/bash", "bash", "-c", command, NULL);
        
        // Si execl regresa, es que ha habido un fallo grave
        perror("Error al mutar con execl");
        exit(EXIT_FAILURE);
    }

    // --- CÓDIGO DEL PROCESO PADRE ---
    return pid;
}

int main(int argc, char *argv[]) {
    // Validación de parámetros mínimos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s -x <\"comando\"> | -s <fichero> [-b]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int opt;
    char *cmd = NULL;
    char *fichero = NULL;
    
    int x_mode = 0;
    int s_mode = 0;
    int b_mode = 0; // Flag para controlar si se activa el segundo plano/paralelismo

    // Procesamos las opciones mediante getopt
    while ((opt = getopt(argc, argv, "x:s:b")) != -1) {
        switch (opt) {
        case 'x':
            x_mode = 1;
            cmd = optarg;
            break;
        case 's':
            s_mode = 1;
            fichero = optarg;
            break;
        case 'b':
            b_mode = 1; // Se activa el flag si el usuario pasa -b
            break;
        default:
            fprintf(stderr, "Uso: %s -x <\"comando\"> | -s <fichero> [-b]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // =========================================================================
    // APARTADO A: Opción -x (Comando único)
    // =========================================================================
    if (x_mode == 1) {
        pid_t pid = run_command(cmd);
        if (pid > 0) {
            int status;
            waitpid(pid, &status, 0); // Espera síncrona obligatoria
        }
    }   

    // =========================================================================
    // APARTADOS B y C: Opción -s con o sin -b (Fichero de comandos)
    // =========================================================================
    if (s_mode == 1) {
        char linea[1024];
        int num_com = 0;
        FILE *fp;

        // Array de estructuras para controlar la recogida asíncrona (Apartado C)
        registro_t tabla_procesos[MAX_COMANDOS];
        int total_lanzados = 0;

        if ((fp = fopen(fichero, "r")) == NULL) {
            perror("Error al abrir el archivo de comandos");
            return EXIT_FAILURE;
        }

        // --- BUCLE DE LECTURA Y LANZAMIENTO ---
        while (fgets(linea, sizeof(linea), fp) != NULL) {
            // Limpieza estricta de saltos de línea (\n o \r\n de Windows/PDF)
            linea[strcspn(linea, "\r\n")] = '\0';
            
            // Ignoramos líneas vacías o comentarios
            if (strlen(linea) == 0 || linea[0] == '#') continue;

            printf("@@ Running command #%d: %s\n", num_com, linea);

            // Lanzamos el proceso hijo
            pid_t pid = run_command(linea);

            if (pid > 0) {
                if (b_mode == 1) {
                    // [MODO APARTADO C]: Guardamos el PID y el número de orden, y NO hacemos wait.
                    // El bucle continuará inmediatamente a lanzar el siguiente hijo.
                    tabla_procesos[total_lanzados].pid = pid;
                    tabla_procesos[total_lanzados].num_comando = num_com;
                    total_lanzados++;
                } else {
                    // [MODO APARTADO B]: Comportamiento secuencial tradicional. Esperamos ya.
                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) {
                        printf("@@ Command #%d terminated (pid: %d, status: %d)\n", 
                               num_com, pid, WEXITSTATUS(status));
                    }
                }
            }
            num_com++;
        }
        fclose(fp);

        // --- BUCLE DE RECOGIDA ASÍNCRONA (SOLO SI SE ACTIVÓ -b) ---
        if (b_mode == 1 && total_lanzados > 0) {
            int status;
            
            // Hacemos exactamente tantos waitpid como hijos hayamos lanzado en paralelo
            for (int i = 0; i < total_lanzados; i++) {
                // waitpid con -1 y opción 0 duerme al padre eficientemente hasta que CUALQUIER hijo muere
                pid_t terminado = waitpid(-1, &status, 0);

                if (terminado > 0) {
                    int num_asociado = -1;
                    
                    // Buscamos en nuestra tabla de registros a qué número de comando pertenecía este PID
                    for (int j = 0; j < total_lanzados; j++) {
                        if (tabla_procesos[j].pid == terminado) {
                            num_asociado = tabla_procesos[j].num_comando;
                            break;
                        }
                    }

                    // Imprimimos la traza exacta requerida por el enunciado
                    if (WIFEXITED(status)) {
                        printf("@@ Command #%d terminated (pid: %d, status: %d)\n", 
                               num_asociado, terminado, WEXITSTATUS(status));
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}