#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  // Define tipos de datos del sistema
#include <sys/stat.h>   // Contiene definición de struct stat y macros
#include <fcntl.h>      // Banderas de control de open()
#include <unistd.h>     // Contiene read(), write(), close(), symlink(), readlink()

// Función auxiliar de bajo nivel que copia el contenido de los descriptores
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


void copy_regular(char *orig, char *dest)
{
	int fdo, fdd;

	if((fdo = open(orig, O_RDONLY)) == -1){
		perror("Error al abrir el fichero regular de entrada");
		exit(EXIT_FAILURE);
	}
	
	if((fdd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1){
		close(fdo);
		perror("Error al abrir el fichero de salida");
		exit(EXIT_FAILURE);
	}
	
	copy(fdo, fdd);
	close(fdo);
	close(fdd);
}


void copy_link(char *orig, char *dest, off_t size)
{
	// a. Reservar memoria para la ruta apuntada (+1 para el '\0')
	char *buffer_ruta = malloc(size + 1);
	if (buffer_ruta == NULL) {
		fprintf(stderr, "Error: No hay memoria suficiente (malloc)\n");
		exit(EXIT_FAILURE);
	}

	// Leer a qué ruta apunta el enlace original
	ssize_t bytes_leidos = readlink(orig, buffer_ruta, size);
	if (bytes_leidos == -1) {
		perror("Error al leer el enlace simbólico (readlink)");
		free(buffer_ruta);
		exit(EXIT_FAILURE);
	}
	
	// Añadir manualmente el carácter nulo al final
	buffer_ruta[bytes_leidos] = '\0'; 

	// c. Crear el nuevo enlace simbólico apuntando a esa ruta
	if (symlink(buffer_ruta, dest) == -1) {
		perror("Error al crear el nuevo enlace simbólico (symlink)");
		free(buffer_ruta);
		exit(EXIT_FAILURE);
	}

	free(buffer_ruta);
	printf("Enlace simbólico duplicado con éxito desde función.\n");
}

int main(int argc, char *argv[])
{
	struct stat infoF;

	if(argc != 3){
        fprintf(stderr, "Uso: %s <fichero_a_copiar> <fichero_copia>\n", argv[0]);
        return EXIT_FAILURE;
    }

	// Analizamos los metadatos del origen sin seguir el enlace
	if(lstat(argv[1], &infoF) == -1){
		perror("Error al ejecutar lstat sobre el origen");
        return EXIT_FAILURE;
	}

	// El main se convierte en un "enrutador" limpio e inteligente
	if(S_ISREG(infoF.st_mode)){
		copy_regular(argv[1], argv[2]);
	}
	else if(S_ISLNK(infoF.st_mode)){
		// Le pasamos también el tamaño (infoF.st_size) para que la función haga el malloc
		copy_link(argv[1], argv[2], infoF.st_size);
	}
	else {
		fprintf(stderr, "Error: El archivo de origen es un directorio o tipo no soportado.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
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
 *
 * lseek()
 * - Sintaxis: off_t lseek(int fd, off_t offset, int whence);
 * - Uso: Es el equivalente a fseek pero a bajo nivel. Mueve el cursor de lectura/escritura del descriptor 'fd'.
 * - Parámetros: SEEK_SET (inicio), SEEK_CUR (posición actual), SEEK_END (final del archivo).
 * - Valor devuelto: La nueva posición del cursor medida en bytes desde el inicio del archivo, o -1 si falla.
 *
 * lstat()
 * - Sintaxis: int lstat(const char *pathname, struct stat *statbuf);
 * - Uso: Obtiene información y metadatos (tamaño, permisos, inodo, tipo) del archivo apuntado por 'pathname' y los guarda en la estructura 'statbuf'.
 * - Diferencia crítica con stat(): Si el archivo es un enlace simbólico, lstat() obtiene la información DEL ENLACE mismo, no del archivo al que apunta.
 * - Valor devuelto: 0 si tiene éxito obtieniendo los metadatos, o -1 si hay un error.
 *
 * symlink()
 * - Sintaxis: int symlink(const char *target, const char *linkpath);
 * - Uso: Crea un enlace simbólico (un "acceso directo" de Linux) en la ruta 'linkpath' que apunta al archivo o directorio 'target'.
 * - Valor devuelto: 0 si el enlace se crea correctamente, o -1 si ocurre un error (ej. si linkpath ya existe).
 *
 * readlink()
 * - Sintaxis: ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
 * - Uso: Lee el contenido de un enlace simbólico (averigua a qué ruta está apuntando realmente) y guarda esa ruta como texto en 'buf'.
 * - Ojo de examen: NO añade el carácter nulo '\0' al final del buffer automáticamente; debes añadirlo tú usando el valor devuelto.
 * - Valor devuelto: El número de bytes colocados en el buffer 'buf', o -1 si ocurre un error.
 *
 * opendir()
 * - Sintaxis: DIR *opendir(const char *name);
 * - Uso: Abre el directorio especificado en 'name' y devuelve un puntero a un flujo de directorio (DIR *) para poder examinar su contenido.
 * - Valor devuelto: Un puntero de tipo DIR * listo para ser usado, o NULL si el directorio no existe o no hay permisos.
 *
 * readdir()
 * - Sintaxis: struct dirent *readdir(DIR *dirp);
 * - Uso: Lee, uno a uno, los elementos (archivos, subdirectorios, enlaces) que contiene el directorio abierto por opendir().
 * - Estructura devuelta: Devuelve un puntero a una 'struct dirent', cuyo campo más importante es 'd_name' (el nombre del archivo/carpeta).
 * - Control de bucle: Avanza automáticamente en cada llamada. Devuelve NULL cuando se llega al final del directorio o si hay un error.
 */

/*
 * Librerías obligatorias para las llamadas al sistema (Bajo Nivel):
 *
 * #include <sys/types.h>  // Define tipos de datos del sistema como off_t, mode_t, pid_t y ssize_t
 * #include <sys/stat.h>   // Necesaria para las constantes de open() y la estructura de lstat()
 * #include <fcntl.h>      // Contiene las banderas de control de open() (O_RDONLY, O_WRONLY, O_CREAT...)
 * #include <unistd.h>     // Esencial: contiene read(), write(), close(), lseek(), symlink() y readlink()
 * #include <dirent.h>     // Necesaria para el manejo de directorios: opendir(), readdir() y DIR *
 */

/*
  # 1. Crear el enlace simbólico "mylink" que apunta al Makefile de otro ejercicio.
ln -s ../ejercicio1/Makefile mylink

# Explicación:
# ln -s [destino] [enlace]
# -s => enlace simbólico
# Esta orden crea 'mylink' apuntando a '../ejercicio1/Makefile'.
# Tras la operación, 'mylink' será un archivo especial (symlink) que actúa como acceso indirecto al Makefile.
# Puedes comprobarlo con:
ls -l mylink
# Te mostrará algo tipo:
# lrwxrwxrwx 1 usuario grupo tamaño fecha mylink -> ../ejercicio1/Makefile

# 2. Copiar el enlace simbólico usando el programa copy2:
./copy2 mylink mylinkcopy

# Explicación:
# - El programa detecta con lstat que 'mylink' es un enlace simbólico.
# - Luego, lee la ruta apuntada ('../ejercicio1/Makefile') usando readlink.
# - Finalmente, crea otro symlink ('mylinkcopy') que apunta a ese mismo destino con symlink.
# Si todo va bien, 'mylinkcopy' será otro enlace simbólico idéntico.

# 3. Comprobar el resultado:
ls -l mylink mylinkcopy
# Salida esperada:
# lrwxrwxrwx ... mylink      -> ../ejercicio1/Makefile
# lrwxrwxrwx ... mylinkcopy -> ../ejercicio1/Makefile

# Explicación:
# Ambos archivos ('mylink', 'mylinkcopy') son enlaces simbólicos que apuntan al mismo destino.
# Puedes confirmar que NO se ha copiado el contenido del Makefile, sino el propio enlace: si el Makefile cambia, ambos enlaces siguen apuntando al mismo fichero.

# 4. Leer el contenido a través del enlace para confirmar que funciona:
cat mylinkcopy

# Explicación:
# 'cat' sigue el enlace simbólico y muestra el contenido del fichero Makefile original.
# Si borrases el Makefile, 'cat mylinkcopy' fallaría porque el destino apuntado ya no existe.

# 5. Compara el contenido de los enlaces y el original:
diff ../ejercicio1/Makefile mylinkcopy

# Explicación:
# diff sigue el enlace, compara el original y el apuntado, y muestra que ambos son idénticos.

# Resumen:
# El procedimiento demuestra tanto la creación como la copia adecuada de enlaces simbólicos, siguiendo exactamente las rutas destino originales.
# Si usases 'copy.c' (ejercicio 1) sobre 'mylink', obtendrías un fichero regular con el contenido del Makefile original, perdiendo la propiedad simbólica.
*/