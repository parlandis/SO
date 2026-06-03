#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor
 *
 * The loadstr() function must allocate memory from the heap to store
 * the contents of the string read from the FILE.
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc())
 *
 * Returns: !=NULL if success, NULL if error
 */
char *loadstr(FILE *file)
{
	char c;
	size_t cont  = 1; 

	while((c = fgetc(file))!= EOF) {
		if(c == '\0' ) break;
		cont++;
	}

	if(c == EOF) return NULL;

	char *buff = malloc(cont);
	size_t bytesR ;
	

	fseek(file, -cont , SEEK_CUR );

	if((bytesR = fread(buff, 1, cont, file) )!= cont){
		perror("Error al leer la linea\n");
		exit(EXIT_FAILURE);
	}

	return buff;	
}

int main(int argc, char *argv[])
{
	if(argc != 2){
		perror("Error con los args");
		exit(EXIT_FAILURE);
	}

	FILE *file = NULL;
	file = fopen(argv[1],"rb");
	if(file == NULL){
		perror("Error al abrir el archivo");
		exit(EXIT_FAILURE);
	}

	char *linea = loadstr(file);

	while(linea != NULL){
		printf("%s\n", linea);
		free(linea);
		linea = loadstr(file);
	}

	free(linea);
	fclose(file);

	return 0;
}



/*
ejec :./write_strings out London Paris Madrid Barcelona Berlin Lisbon

* FUNCIONES USADAS Y SU PROPÓSITO:

 * fopen()
 * - Sintaxis: FILE *fopen(const char *pathname, const char *mode);
 * - Uso: Abre el archivo especificado en la ruta y le asocia un flujo de datos.
 * - En este caso, se suele usar en modo lectura binaria ("rb") o escritura binaria ("wb").
 * - Valor devuelto: Puntero al archivo (FILE *) si tiene éxito, o NULL si ocurre un error.
 *
 * fclose()
 * - Sintaxis: int fclose(FILE *stream);
 * - Uso: Cierra el flujo del archivo y asegura que todos los datos del buffer se guarden en el disco.
 * - Aquí se usa al final de la ejecución para liberar el descriptor del archivo.
 * - Valor devuelto: 0 si se cerró correctamente, o EOF (-1) si hubo un error.
 *
 * fread()
 * - Sintaxis: size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 * - Uso: Lee datos en bloque desde un archivo y los almacena en un buffer.
 * - En este caso, se lee de uno en uno: size=1, nmemb=1 (o el tamaño total calculado).
 * - Valor devuelto: Nº de bytes leídos (si lees de uno en uno, solo puede ser 0 o 1).
 *
 * fwrite()
 * - Sintaxis: size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
 * - Uso: Escribe datos de un buffer hacia un archivo o stdout.
 * - Aquí se escribe de uno en uno: size=1, nmemb=1 (o el bloque completo).
 * - Valor devuelto: Nº de bytes escritos (debería coincidir con lo solicitado).
 *
 *  strlen()
 * - Sintaxis: size_t strlen(const char *s);
 * - Uso: Calcula la longitud de una cadena de caracteres (string).
 * - En este caso, cuenta todos los caracteres hasta encontrar el carácter nulo de terminación ('\0'), el cual NO se incluye en la cuenta.
 * - Valor devuelto: Nº de bytes (caracteres) que tiene la cadena.
 * fseek()
 * - Sintaxis: int fseek(FILE *stream, long offset, int whence);
 * - Uso: Mueve el indicador de posición (cursor) del archivo a un lugar específico.
 * - En este caso, se usa con SEEK_END y offset=0 para ir al final y poder medir el archivo.
 * - Valor devuelto: 0 si se posiciona con éxito, o -1 si ocurre un error.
 *
 * malloc()
 * - Sintaxis: void *malloc(size_t size);
 * - Uso: Reserva un bloque de memoria dinámica en el Heap del tamaño exacto en bytes solicitado.
 * - Aquí se usa para crear un buffer a medida tras conocer el tamaño del archivo con ftell.
 * - Valor devuelto: Puntero a la dirección de memoria reservada, o NULL si el sistema se quedó sin memoria.
 */

 /*
 * FUNCIONES USADAS Y SU PROPÓSITO:
 *
 * fopen  - Abre el archivo para lectura binaria ("rb"). Necesario para operar sobre archivos.
 *
 * fgetc  - Lee un carácter (byte) desde el archivo. Se usa para contar el número de caracteres de cada cadena.
 *
 * ftell  - Retorna la posición actual del puntero del archivo; esencial para volver atrás y poder leer toda la cadena.
 *
 * fseek  - Permite reposicionar el puntero del archivo; se usa para volver al inicio de la cadena detectada.
 *
 * malloc - Reserva memoria dinámica proporcional al tamaño de la cadena encontrada.
 *
 * fread  - Lee de golpe la cadena completa (todos sus bytes más el terminador nulo).
 *
 * printf - Imprime la cadena leída (requiere que esté correctamente terminada).
 *
 * free   - Libera la memoria dinámica reservada por malloc para evitar fugas de memoria.
 *
 * fclose - Cierra el archivo abierto y libera recursos.
 * 
 *
 * Este programa recupera cadenas de longitud variable guardadas consecutivamente,
 * cada una finalizada con '\0'. Para cada cadena, se reserva memoria, se lee, se imprime,
 * y luego se libera la memoria para asegurar buen uso de los recursos del sistema.
 */
