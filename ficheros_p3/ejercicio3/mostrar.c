#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  // Define tipos de datos del sistema
#include <sys/stat.h>   // Contiene definición de struct stat y macros
#include <fcntl.h>      // Banderas de control de open()
#include <unistd.h>     // Contiene read(), write(), close(), lseek(), getopt...

#define BUF_SIZE 512

int main(int argc, char *argv[])
{
	int opt;
	int N = 0; // SOLUCIÓN: Inicializado a 0 por si -n no aparece
	int E = 0; // Flag para indicar presencia de -e
	int fd;
	
	// 1. Parseo de argumentos con getopt
	while((opt = getopt(argc, argv, "en:")) != -1){
		switch (opt) {
			case 'n':
				N = atoi(optarg);
				break;
			case 'e':
				E = 1;
				break;
			default:
				fprintf(stderr, "Uso: %s [-n N] [-e] <fichero>\n", argv[0]);
				return EXIT_FAILURE;
		}
	}

	// 2. Control de argumentos posicionales (Debe quedar exactamente 1: el fichero)
	if(argc - optind != 1){
		fprintf(stderr, "Error: Debe especificar exactamente un archivo de entrada.\n");
		fprintf(stderr, "Uso: %s [-n N] [-e] <fichero>\n", argv[0]);
		return EXIT_FAILURE;
	}

	// 3. Apertura del archivo a bajo nivel (SOLUCIÓN: Paréntesis de prioridad añadidos)
	if ((fd = open(argv[optind], O_RDONLY)) == -1){
		perror("Error al abrir el archivo");
		return EXIT_FAILURE;
	}

	// 4. Posicionamiento del marcador según los flags con lseek()
	if (E == 1) {
		// Si está -e, nos posicionamos N bytes antes del final del fichero
		if (lseek(fd, -N, SEEK_END) == -1) {
			perror("Error al ejecutar lseek (SEEK_END)");
			close(fd);
			return EXIT_FAILURE;
		}
	} else {
		// Si no está -e, saltamos los primeros N bytes desde el comienzo
		if (lseek(fd, N, SEEK_SET) == -1) {
			perror("Error al ejecutar lseek (SEEK_SET)");
			close(fd);
			return EXIT_FAILURE;
		}
	}

	// 5. Lectura y escritura en la salida estándar
	char byte;
	ssize_t bytes_leidos;
	
while((bytes_leidos = read(fd, &byte, 1)) > 0){
    	ssize_t bytes_escritos =  write(1, &byte, 1);

		if(bytes_escritos != bytes_leidos){
			perror("Error al escribir en la salida estándar");
			close(fd);
			return EXIT_FAILURE;
		}
	}

	if (bytes_leidos == -1) {
        perror("Error leyendo del fichero origen");
        close(fd);
        return EXIT_FAILURE;
    }

	// 6. Cierre del descriptor
	close(fd);
	return EXIT_SUCCESS;
}

/*
 * getopt()
 * - Sintaxis: int getopt(int argc, char *const argv[], const char *optstring);
 * - Uso: Analiza de forma iterativa las opciones y banderas pasadas por línea de comandos (argumentos que empiezan por '-').
 * - En este caso ("en:"): 
 * - 'e' es una bandera simple (sin argumentos). Si aparece, activamos un flag indicador (E = 1).
 * - 'n:' va seguido de dos puntos, lo que significa que requiere obligatoriamente un argumento (el valor numérico N).
 * - Variable optarg: Puntero al texto del argumento que acompaña a la opción (ej: si pasas "-n 10", optarg apunta a "10").
 * - Variable optind: Índice del siguiente elemento a procesar en argv. Al salir del bucle while, apunta al primer argumento posicional (el nombre del fichero).
 *
 * atoi()
 * - Sintaxis: int atoi(const char *nptr);
 * - Librería: <stdlib.h>
 * - Uso: Convierte una cadena de caracteres (string) que contiene números en un valor entero de C (int).
 * - En este caso: Se usa para transformar el argumento de texto capturado por getopt (optarg) en el número entero N.
 *
 * open()
 * - Sintaxis: int open(const char *pathname, int flags);
 * - Librería: <fcntl.h>
 * - Uso: Abre un archivo a bajo nivel (comunicación directa con el Kernel) y asigna un descriptor de fichero (un entero indexado).
 * - Flag O_RDONLY: Abre el archivo en modo "Solo lectura".
 * - Prioridad Crítica (Ojo de examen): La asignación debe envolverse en paréntesis siempre: 'if ((fd = open(...)) == -1)'. Si no se hace, el operador '==' se ejecuta antes y corrompe el descriptor guardando un 0 o un 1.
 *
 * lseek()
 * - Sintaxis: off_t lseek(int fd, off_t offset, int whence);
 * - Librería: <unistd.h>
 * - Uso: Desplaza el marcador o cursor de posición de lectura/escritura del descriptor de fichero 'fd' los bytes indicados en 'offset'.
 * - Modos de posicionamiento (whence):
 * - SEEK_SET: Mueve el cursor 'offset' bytes partiendo desde el PRINCIPIO del archivo.
 * - SEEK_END: Mueve el cursor 'offset' bytes partiendo desde el FINAL del archivo (para retroceder se pasa el offset en negativo, ej: -N).
 * - Valor devuelto: La posición resultante en bytes desde el inicio si tiene éxito, o -1 si ocurre un error (ej. intentar lseek sobre tuberías o el teclado).
 *
 * read()
 * - Sintaxis: ssize_t read(int fd, void *buf, size_t count);
 * - Librería: <unistd.h>
 * - Uso: Lee hasta un máximo de 'count' bytes desde el archivo ('fd') y los almacena en la dirección de memoria apuntada por 'buf'.
 * - Valor devuelto: Número de bytes que ha conseguido leer con éxito. Devuelve 0 cuando alcanza el final del archivo (EOF) y -1 en caso de error.
 *
 * write()
 * - Sintaxis: ssize_t write(int fd, const void *buf, size_t count);
 * - Librería: <unistd.h>
 * - Uso: Escribe de forma directa 'count' bytes desde el buffer 'buf' hacia el descriptor de destino 'fd'.
 * - Salida estándar (fd = 1): En sistemas UNIX/Linux, el descriptor número 1 está reservado de forma fija para la pantalla (stdout). Escribir en el fd 1 saca el texto por la terminal.
 * - Validación: Se debe comprobar siempre que 'bytes_escritos == bytes_leidos' para garantizar que no se ha cortado la escritura (ej. por disco lleno).
 *
 * close()
 * - Sintaxis: int close(int fd);
 * - Librería: <unistd.h>
 * - Uso: Cierra el descriptor de fichero activo, liberando esa tabla de canales del proceso para que el Kernel pueda reutilizarla.
 */