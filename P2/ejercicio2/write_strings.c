#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

int main(int argc, char* argv[])
{
	if(argc < 3){
		printf("Error");
		exit(0);
	}

	FILE *file = NULL;

	file = fopen(argv[1], "wb");
	if(file == NULL){
		perror("Error al abrir el archivo\n");
		exit(EXIT_FAILURE);
	}

	for(int i = 2;  i < argc; i++){
		size_t nBytes = strlen(argv[i]) + 1; //Por el '\0' del final de todos los strings  
		if(nBytes != fwrite(argv[i], 1, nBytes, file)){
			perror("Error al escribir en el archivo\n");
			exit(EXIT_FAILURE);
		}
	}


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