#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#define UMBRAL_DEFECTO 300 // 300 KB por defecto

/*
 * es_imagen_png
 * - Comprueba si el nombre del archivo termina con la extensión ".png".
 */
int es_imagen_png(const char *nombre) {
    size_t len = strlen(nombre);
    if (len > 4 && strcmp(nombre + len - 4, ".png") == 0) {
        return 1;
    }
    return 0;
}

/*
 * procesar_y_clasificar
 * - Abre el directorio actual, filtra los ficheros .png y crea hijos para enlazarlos.
 */
void procesar_y_clasificar(const char *path_f1, const char *path_f2, long umbral_bytes) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    pid_t pid;
    int hijos_lanzados = 0;

    // Abrimos el directorio actual "."
    if ((dir = opendir(".")) == NULL) {
        perror("Error al abrir el directorio actual");
        exit(EXIT_FAILURE);
    }

    // Recorremos secuencialmente todas las entradas del directorio
    while ((entry = readdir(dir)) != NULL) {
        
        // Filtramos para trabajar únicamente con ficheros que tengan extensión .png
        if (es_imagen_png(entry->d_name)) {
            
            // Consultamos la información de i-nodo del archivo para conocer su tamaño real
            if (stat(entry->d_name, &st) == -1) {
                perror("Error en stat");
                continue;
            }

            // Saltamos si por algún motivo la entrada no fuese un archivo regular
            if (!S_ISREG(st.st_mode)) continue;

            hijos_lanzados++;

            // [Apartado A] Generamos un proceso hijo independiente para el enlazado rígido
            pid = fork();
            if (pid < 0) {
                perror("Error en fork");
                exit(EXIT_FAILURE);
            } 
            else if (pid == 0) {
                // --- CÓDIGO DEL PROCESO HIJO ---
                char ruta_destino[512];
                
                // Clasificamos según el tamaño umbral en bytes calculado
                if (st.st_size > umbral_bytes) {
                    snprintf(ruta_destino, sizeof(ruta_destino), "%s/%s", path_f1, entry->d_name);
                } else {
                    snprintf(ruta_destino, sizeof(ruta_destino), "%s/%s", path_f2, entry->d_name);
                }

                // Intentamos crear el Hard Link mediante la llamada al sistema link()
                if (link(entry->d_name, ruta_destino) == -1) {
                    perror("Error al crear el enlace rígido (link)");
                    exit(EXIT_FAILURE);
                }

                printf("Link to %s created successfully\n", ruta_destino);
                exit(EXIT_SUCCESS); // El hijo finaliza tras cumplir su tarea
            }
        }
    }
    closedir(dir);

    // [EXAMEN] Espera síncrona/asíncrona de recogida de la telemetría de todos los hijos creados
    int status;
    for (int i = 0; i < hijos_lanzados; i++) {
        pid_t pid_hijo = wait(&status);
        if (pid_hijo > 0) {
            if (WIFEXITED(status)) {
                // Finalización natural sin fallos de segmentación
                // Se puede añadir traza si el enunciado lo requiere
            }
        }
    }
    printf("%d imagenes procesadas de forma paralela.\n", hijos_lanzados);
}

int main(int argc, char *argv[]) {
    // Parámetros por defecto para cumplir con el apartado A si no se pasan flags
    char *folder_01 = "./Folder_01";
    char *folder_02 = "./Folder_02";
    long umbral_kb = UMBRAL_DEFECTO;

    int opt;
    // [Apartado B] Parseo dinámico de los argumentos configurados con getopt
    // -l: path Folder_01, -s: path Folder_02, -t: valor del umbral
    while ((opt = getopt(argc, argv, "l:s:t:")) != -1) {
        switch (opt) {
            case 'l':
                folder_01 = optarg;
                break;
            case 's':
                folder_02 = optarg;
                break;
            case 't':
                umbral_kb = atol(optarg);
                break;
            default:
                fprintf(stderr, "Uso: %s [-l path_f1] [-s path_f2] [-t umbral_kb]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Convertimos el umbral de Kilobytes a Bytes puros para comparar con st.st_size (1 KB = 1024 Bytes)
    long umbral_bytes = umbral_kb * 1024;

    // Lanzamos la subrutina de clasificación y multiprocesamiento
    procesar_y_clasificar(folder_01, folder_02, umbral_bytes);

    return EXIT_SUCCESS;
}
