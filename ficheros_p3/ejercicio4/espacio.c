#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  // Define tipos de datos del sistema
#include <sys/stat.h>   // Contiene definición de struct stat y macros
#include <fcntl.h>      // Banderas de control de open()
#include <unistd.h>     // Contiene read(), write(), close(), lseek(), getopt...
#include <dirent.h>    // Para funciones de manejo de directorios como opendir y readdir.
#include <string.h>

/* Forward declaration */
int get_size_dir(char *fname, size_t *blocks);

/* Gets in the blocks buffer the size of file fname using lstat. If fname is a
 * directory get_size_dir is called to add the size of its contents.
 */
int get_size(char *fname, size_t *blocks)
{	
		struct stat infoF;
		if(lstat(fname, &infoF) == -1){
			printf("Error al ejecutar lstat sobre %s", fname);
			return EXIT_FAILURE;
		}

		if(S_ISREG(infoF.st_mode)){
			*blocks += infoF.st_blocks;
		}
		else {
			get_size_dir(fname, blocks);
		}
}


/* Gets the total number of blocks occupied by all the files in a directory. If
 * a contained file is a directory a recursive call to get_size_dir is
 * performed. Entries . and .. are conveniently ignored.
 */
int get_size_dir(char *dname, size_t *blocks)
{
	struct stat infoD;
	if(lstat(dname, &infoD) == -1){
			printf("Error al ejecutar lstat sobre %s", dname);
			return EXIT_FAILURE;
	}
	
	if(S_ISDIR(infoD.st_mode)){
		struct dirent *entrada;
		DIR *dir = opendir(dname);

		if(dir == NULL){
			printf("Error al abrir el directorio %s", dname);
			return -1;
		}

		while((entrada = readdir(dir)) != NULL){
			// ⚠️ ALERTA DE EXAMEN: Filtrar siempre "." y ".."
			// Todos los directorios de Linux contienen estas dos carpetas ocultas.
			// Si haces un programa que recorre carpetas recursivamente y no las filtras,
			// entrarás en un bucle infinito.
			if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
				continue; // Saltamos este elemento y vamos al siguiente
			}

			get_size(entrada->d_name, blocks);
		}
	}
	else {
		printf("No es un directorio: %s", dname);
		return -1;
	}


}

/* Processes all the files in the command line calling get_size on them to
 * obtain the number of 512 B blocks they occupy and prints the total size in
 * kilobytes on the standard output
 */
int main(int argc, char *argv[])
{

	size_t totalBlocks = 0; 


	for(int i = 1; i <= argc; i++ ){
		totalBlocks += get_size(argv[i], &totalBlocks);
	}


	return 0;
}
