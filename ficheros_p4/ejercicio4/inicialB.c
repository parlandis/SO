#include <stdio.h>      // Entrada/Salida estándar: printf, fprintf, perror, fgets, etc.
#include <stdlib.h>     // Librería estándar: gestión de memoria (malloc/free), conversión (atoi) y salida (exit).
#include <unistd.h>     // API POSIX estándar: fork, execvp, alarm, pause, sleep, pipe, read, write, close.
#include <fcntl.h>      // Control de archivos: define constantes para 'open' (O_CREAT, O_RDONLY, etc.).
#include <sys/types.h>  // Tipos de datos del sistema: definiciones como pid_t, size_t, uid_t, etc.
#include <sys/wait.h>   // Espera de procesos: wait, waitpid y macros de estado (WIFEXITED, WEXITSTATUS).
#include <string.h>     // Manipulación de cadenas: strcpy, strlen, strtok, strcmp, strerror.

int main(void)
{
    int fd1,fd2,i,pos;
    char c;
    char buffer[6];
     // =================================================================
    // CREAR Y ABRIR EL ARCHIVO
    // =================================================================

    // Padre abre el archivo una sola vez
    // Lo mantiene abierto durante todo el proceso
    // Abrir/crear archivo con permisos de lectura/escritura
    // O_CREAT: Crear si no existe
    // O_TRUNC: Truncar a longitud 0 si existe (borrar contenido)
    // O_RDWR: Abrir para lectura y escritura
    // S_IRUSR | S_IWUSR: Permisos rw------- (usuario puede leer y escribir)
    fd1 = open("output.txt", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);



    pid_t hijo;

    // =================================================================
    // FASE: INTERCALAR ESCRITURAS PADRE-HIJO
    // =================================================================

    // Iterar 10 veces: una por cada par (padre-hijo)
    for (i=1; i < 10; i++) {
        write(fd1, "00000", 5);
        pos = lseek(fd1, i *5, SEEK_CUR);
        hijo= fork();
        if (hijo== 0) {
            /* Child */
            sprintf(buffer, "%d", i*11111);
            lseek(fd1, pos, SEEK_SET);
            write(fd1, buffer, 5);
            close(fd1);
            exit(0);
        } else {
            /* Parent */
            write(fd1, "00000", 5);
            lseek(fd1, 10, SEEK_CUR);
        }
    }

	//wait for all childs to finish
    while (wait(NULL) != -1);

    lseek(fd1, 0, SEEK_SET);
    printf("File contents are:\n");
    while (read(fd1, &c, 1) > 0)
        printf("%c", (char) c);
    printf("\n");
    close(fd1);
    exit(0);
}
