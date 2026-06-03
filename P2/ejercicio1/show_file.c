#include <stdio.h>  // FILE* stdin...
#include <stdlib.h>  // malloc , free 
#include <err.h>

int main(int argc, char* argv[]) {
	FILE* file=NULL;
	size_t bytesRead; 
	char buffer[1];

	if (argc!=2) {
		fprintf(stderr,"Usage: %s <file_name>\n",argv[0]);
		exit(1);
	}

	/* Open file */
	if ((file = fopen(argv[1], "r")) == NULL)
		err(2,"The input file %s could not be opened",argv[1]);

	/* Read file byte by byte */
	//size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
	
	while((bytesRead = fread(buffer, 1, 1, file)) != 0){
		if(fwrite(buffer, 1, 1, stdout) != bytesRead){
			fclose(file);
			perror("Error al escribir el stdout con fwrite\n");
			exit(EXIT_FAILURE);
		}
	}

	fclose(file);
	return 0;
}

/*
 * FREAD & FWRITE - Lectura y escritura de archivos BYTE A BYTE en C
 *
 * fread()
 *   - Sintaxis: size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 *   - Uso: Lee datos en bloque desde un archivo y los almacena en un buffer.
 *   - En este caso, se lee de uno en uno: size=1, nmemb=1.
 *   - Valor devuelto: Nº de bytes leídos (aquí solo puede ser 0 o 1).
 *
 * fwrite()
 *   - Sintaxis: size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
 *   - Uso: Escribe datos de un buffer hacia un archivo o stdout.
 *   - Aquí se escribe de uno en uno: size=1, nmemb=1.
 *   - Valor devuelto: Nº de bytes escritos (debería coincidir con fread).
 *
 * Lógica principal:
 *   - Abrir el archivo para lectura con fopen().
 *   - Leer el archivo byte por byte con fread().
 *   - Cada byte leído se imprime en pantalla usando fwrite().
 *   - Comprobar que fwrite escribió correctamente cada byte.
 *   - Cerrar el archivo con fclose().
 *
 * Diferencia clave:
 *   - A diferencia de getc/putc (que procesan caracteres), fread/fwrite trabajan directamente con bloques de memoria (aunque aquí el bloque es de un solo byte).
 *   - Este método es especialmente útil para manipular datos binarios, pero también funciona para texto.
 *
 */