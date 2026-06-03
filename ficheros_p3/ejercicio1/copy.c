#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  // Define tipos de datos del sistema como off_t, mode_t, pid_t y ssize_t
#include <sys/stat.h>   // Necesaria para las constantes de open() y la estructura de lstat()
#include <fcntl.h>      // Contiene las banderas de control de open() (O_RDONLY, O_WRONLY, O_CREAT...)
#include <unistd.h>     // Esencial: contiene read(), write(), close(), lseek(), symlink() y readlink()
#include <dirent.h>     // Necesaria para el manejo de directorios: opendir(), readdir() y DIR *



void copy(int fdo, int fdd)
{
	char buffer[512];
	ssize_t contRead, contWrite;

	while((contRead = read(fdo, buffer, 512)) != 0){
		if((contWrite = write(fdd, buffer, contRead) )!= contRead){
			fprintf(stderr, "Error al escribir\n");
			return;
		}

	}
}

int main(int argc, char *argv[])
{
	int fdo, fdd;
	if(argc != 3){
        fprintf(stderr, "Uso: %s <fichero_a_copiar> <fichero_copia>\n", argv[0]);
        return EXIT_FAILURE;
    }

	// Abrimos el archivo origen con open en modo solo lectura (O_RDONLY).
    // open devuelve el descriptor (int) o -1 si hay error.
	if((fdo = open(argv[1], O_RDONLY)) == -1){
		fprintf(stderr, "Error al abrir el fichero de entrada\n");
		return EXIT_FAILURE;
	}

	// Abrimos/creamos el archivo destino: modo solo escritura, lo crea si no existe (O_CREAT) y lo vacía si existe (O_TRUNC).
    // 0644 => permisos rw-r--r--
    if((fdd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) ==-1){
		close(fdo);
		fprintf(stderr, "Error al abrir el fichero de salida\n");
		return EXIT_FAILURE;
	}

	copy(fdo, fdd);

	close(fdo);
	close(fdd);

	return 0;
}


/*
 * open()
 * - Sintaxis: int open(const char *pathname, int flags, ... /* mode_t mode *\/);
 * - Uso: Abre el archivo especificado en 'pathname' a bajo nivel y devuelve un descriptor de fichero (fd).
 * - Flags comunes: O_RDONLY (lectura), O_WRONLY (escritura), O_CREAT (crear si no existe), O_TRUNC (vaciar).
 * - Valor devuelto: Un entero mayor o igual a 0 (el fd asignado) si tiene éxito, o -1 si ocurre un error.
 *
 * read()
 * - Sintaxis: ssize_t read(int fd, void *buf, size_t count);
 * - Uso: Lee hasta 'count' bytes directamente desde el descriptor de fichero 'fd' y los guarda en el buffer 'buf'.
 * - Valor devuelto: Nº de bytes leídos. Devuelve 0 si llega al final del archivo (EOF) y -1 si hay un error. 
 * - Tipo de retorno: ssize_t (entero con signo, necesario para poder devolver el -1 de error).
 *
 * write()
 * - Sintaxis: ssize_t write(int fd, const void *buf, size_t count);
 * - Uso: Escribe hasta 'count' bytes desde el buffer 'buf' directamente en el descriptor de fichero 'fd' o salida estándar (fd=1).
 * - Valor devuelto: Nº de bytes escritos con éxito en el destino, o -1 si ocurre un error (ej. disco lleno).
 *
 * close()
 * - Sintaxis: int close(int fd);
 * - Uso: Libera y cierra el descriptor de fichero 'fd' para que el sistema operativo pueda asignárselo a otro proceso.
 * - Valor devuelto: 0 si se cerró correctamente, o -1 si hubo un error.
*/