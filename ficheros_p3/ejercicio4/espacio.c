#include <stdio.h>      // printf, fprintf, perror
#include <stdlib.h>     // exit, EXIT_*, malloc, free
#include <unistd.h>     // read, write, close, readlink, symlink
#include <sys/stat.h>   // struct stat, lstat, S_ISREG, S_ISLNK
#include <dirent.h>  // opendir , readdir 
#include <string.h>  //strcmp

/* Forward declaration */
int get_size_dir(char *fname, size_t *blocks);

/* Gets in the blocks buffer the size of file fname using lstat. If fname is a
 * directory get_size_dir is called to add the size of its contents.
 */
int get_size(char *fname, size_t *blocks)
{
	struct stat fichero;

	if(lstat(fname,&fichero) == -1){
		perror("Error con lstat");
		exit(EXIT_FAILURE);
	}

	if(S_ISDIR(fichero.st_mode)){ // Si es un directorio...
		return get_size_dir(fname,blocks);
	}else{ // Si no lo es...
		// En caso de que solo sea un fichero, lo calculamos directamente gracias al tipo que nos ha dado lsat (info del archivo)
        *blocks += (fichero.st_blocks * 512) / 1024; // Convert to kilobytes
		return 0;
	}

}


/* Gets the total number of blocks occupied by all the files in a directory. If
 * a contained file is a directory a recursive call to get_size_dir is
 * performed. Entries . and .. are conveniently ignored.
 */
int get_size_dir(char *dname, size_t *blocks)
{
	/*
 * opendir: Abre un directorio.
 * readdir: Lee las entradas del directorio.
 * closedir: Cierra el directorio.
 * Estructura dirent:
 *  - d_name: Nombre del archivo.
 *  - d_ino: Número de i-nodo.
 */
	DIR *d = opendir(dname);
	if(d == NULL){
		perror("Error al abrir el directorio");
		exit(EXIT_FAILURE);
	}
	struct dirent *entrada;
/*
* dirent: estructura definida en <dirent.h> que representa una entrada de direct
* struct dirent {
*    ino_t          d_ino;       // Número de inodo
*    off_t          d_off;       // Desplazamiento al siguiente dirent
*    unsigned short d_reclen;    // Longitud del registro
*    unsigned char  d_type;      // Tipo de archivo (DT_REG; DT_DIR; DT_LNK; DT_BLK; DT_UNKNOWN
*    char           d_name[];    // Nombre del archivo (terminado en '\0')
* };
*/
	while((entrada = readdir(d)) != NULL){
		if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue; //Si es el . o .. pasa de ellos
        }
		char *path =("%s/%s",dname,entrada->d_name);
		if(get_size(path,blocks) == -1){
			closedir(d);
			perror("Error con el archivo");
			return -1;
		}
	}
	closedir(d);
	return 0;
}

/* Processes all the files in the command line calling get_size on them to
 * obtain the number of 512 B blocks they occupy and prints the total size in
 * kilobytes on the standard output
 */
int main(int argc, char *argv[])
{
	if(argc < 2 ){
			printf("Usage: %s nombreFichero\n",argv[0]);
			exit(EXIT_FAILURE);
		}
	size_t total_blocks = 0, aux=0;

    for (int i = 1; i < argc; i++) {
		aux=total_blocks;
		if (get_size(argv[i], &total_blocks) == -1) {
    		fprintf(stderr, "Error processing file: %s\n", argv[i]);
		} else {
			aux=total_blocks-aux;
    		// Aquí imprimimos el tamaño y el nombre del archivo si no hay errores
    		printf("%luK %s\n", aux, argv[i]);
		}
    }

    printf("%luK total\n", total_blocks);
    return EXIT_SUCCESS;

	return 0;
}
/*
ejec : ./show_file prueba.txt

*/
/* * =================================================================================
 * RESUMEN DE FUNCIONES DEL SISTEMA (Manual de referencia rápida)
 * =================================================================================
 *
 * 1. lstat(const char *pathname, struct stat *statbuf);
 * ---------------------------------------------------------------------------------
 * DESCRIPCIÓN:
 * Obtiene información sobre el fichero apuntado por 'pathname'. 
 * A diferencia de 'stat', si 'pathname' es un enlace simbólico, obtiene 
 * la información del propio enlace, no del fichero al que apunta.
 * * PARÁMETROS:
 * - pathname: Ruta al fichero (cadena de caracteres).
 * - statbuf:  Puntero a un struct donde se guardará la información.
 *
 * CAMPOS ÚTILES DE 'struct stat':
 * - st_blocks: Número de bloques de 512 bytes asignados al fichero.
 * - st_mode:   Contiene el tipo de fichero y permisos. 
 * Usa la macro S_ISDIR(statbuf.st_mode) para ver si es directorio.
 *
 * RETORNO:
 * 0 si hubo éxito, -1 si hubo error (y fija 'errno').
 *
 *
 * 2. opendir(const char *name);
 * ---------------------------------------------------------------------------------
 * DESCRIPCIÓN:
 * Abre un flujo de directorio correspondiente al directorio 'name' y devuelve
 * un puntero a dicho flujo. El flujo se posiciona en la primera entrada.
 *
 * PARÁMETROS:
 * - name: Ruta del directorio a abrir.
 *
 * RETORNO:
 * Un puntero de tipo (DIR *) si hubo éxito.
 * NULL si hubo error (ej. permiso denegado, no existe).
 *
 *
 * 3. readdir(DIR *dirp);
 * ---------------------------------------------------------------------------------
 * DESCRIPCIÓN:
 * Devuelve un puntero a una estructura 'dirent' que representa la siguiente
 * entrada de directorio en el flujo apuntado por 'dirp'.
 * Avanza la posición en el flujo automáticamente.
 *
 * CAMPOS ÚTILES DE 'struct dirent':
 * - d_name: Nombre del fichero (ej: "foto.jpg", ".", "..").
 *
 * RETORNO:
 * Puntero a 'struct dirent' con la siguiente entrada.
 * NULL si se llega al final del directorio o si hay error.
 * =================================================================================
 */
