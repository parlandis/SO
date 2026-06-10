#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt */
#include <string.h>     /* strlen */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */

/*
 * Función: imprimir_uso
 * Descripción: Imprime el modo de uso del programa
 */
void imprimir_uso(char *prog_name) {
    fprintf(stderr, "Uso: %s -f <fichero> [-r] [-w]\n", prog_name);
    fprintf(stderr, "  -f <fichero>  Nombre del fichero a abrir (OBLIGATORIO)\n");
    fprintf(stderr, "  -r            Abrir en modo lectura\n");
    fprintf(stderr, "  -w            Abrir en modo escritura\n");
    fprintf(stderr, "  Nota: Es necesario especificar al menos -r o -w\n");
}

int main(int argc, char *argv[]) {
    int opt;                    /* Variable para getopt */
    char *filename = NULL;      /* Nombre del fichero */
    int read_mode = 0;          /* Flag para modo lectura */
    int write_mode = 0;         /* Flag para modo escritura */
    int fd;                     /* Descriptor de fichero */
    int flags = 0;              /* Flags para open() */
    char buffer[256];           /* Buffer para lectura */
    ssize_t bytes_written;      /* Bytes escritos */
    ssize_t bytes_read;         /* Bytes leídos */
    char *test_data = "Datos de prueba para escribir\n"; /* Datos a escribir */
    
    /*
     * Parsear argumentos con getopt
     * - f: requiere argumento (nombre del fichero)
     * - r: sin argumento (flag de lectura)
     * - w: sin argumento (flag de escritura)
     */
    while ((opt = getopt(argc, argv, "f:rw")) != -1) {
        switch (opt) {
            case 'f':
                /* optarg contiene el valor del argumento después de -f */
                filename = optarg;
                break;
            case 'r':
                /* Activar flag de lectura */
                read_mode = 1;
                break;
            case 'w':
                /* Activar flag de escritura */
                write_mode = 1;
                break;
            case '?':
                /* getopt imprime un mensaje de error automáticamente */
                fprintf(stderr, "Opción no reconocida: -%c\n", optopt);
                imprimir_uso(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    /*
     * VALIDACIONES
     */
    
    /* Verificar que se especificó -f con un fichero */
    if (filename == NULL) {
        fprintf(stderr, "Error: Debe especificar -f <fichero>\n");
        imprimir_uso(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Verificar que se especificó al menos -r o -w */
    if (!read_mode && !write_mode) {
        fprintf(stderr, "Error: Debe especificar al menos -r o -w\n");
        imprimir_uso(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /*
     * CONSTRUIR FLAGS PARA open()
     * 
     * Necesitamos especificar EXACTAMENTE UNO de:
     * - O_RDONLY: solo lectura
     * - O_WRONLY: solo escritura
     * - O_RDWR:   lectura y escritura
     * 
     * Luego podemos añadir flags adicionales con |
     */
    if (read_mode && write_mode) {
        /* Ambos modos: lectura y escritura */
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (read_mode) {
        /* Solo lectura */
        flags = O_RDONLY;
    } else if (write_mode) {
        /* Solo escritura */
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    }
    
    /*
     * Explicación de los flags:
     * - O_CREAT:  Crea el fichero si no existe
     * - O_TRUNC:  Si existe, trunca su contenido (lo borra)
     * - O_WRONLY: Solo escritura (se usa en combinación con O_CREAT)
     * - O_RDONLY: Solo lectura
     * - O_RDWR:   Lectura y escritura
     */
    
    printf("========================================\n");
    printf("Programa apertura.c\n");
    printf("========================================\n");
    printf("Fichero: %s\n", filename);
    printf("Modo: %s%s%s\n",
           read_mode ? "LECTURA" : "",
           (read_mode && write_mode) ? " + " : "",
           write_mode ? "ESCRITURA" : "");
    printf("========================================\n\n");
    
    /*
     * ABRIENDO EL FICHERO
     * 
     * open() tiene tres argumentos:
     * 1. const char *path: ruta del fichero
     * 2. int oflag: flags que especifican cómo abrirlo
     * 3. mode_t mode: permisos (solo relevante si usamos O_CREAT)
     *    
     *    Los permisos se especifican en octal:
     *    S_IRUSR = 0400 (user read)
     *    S_IWUSR = 0200 (user write)
     *    S_IRGRP = 0040 (group read)
     *    S_IROTH = 0004 (others read)
     * 
     * Retorna: file descriptor (número ≥ 0) si éxito, -1 si error
     */
    fd = open(filename, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    if (fd == -1) {
        /* Error al abrir: imprime el error del sistema */
        fprintf(stderr, "Error al abrir/crear el fichero '%s': ", filename);
        perror("open");  /* Imprime el mensaje de error del sistema asociado a errno */
        exit(EXIT_FAILURE);
    }
    
    printf("✓ Fichero abierto exitosamente (fd = %d)\n\n", fd);
    
    /*
     * INTENTAR ESCRIBIR EN EL FICHERO
     * 
     * write() escribe datos en el descriptor de fichero
     * Si el fichero se abrió con O_RDONLY (solo lectura),
     * write() fallará con error EBADF (Bad file descriptor)
     */
    printf("Intentando ESCRIBIR en el fichero...\n");
    bytes_written = write(fd, test_data, strlen(test_data));
    
    if (bytes_written == -1) {
        /* Error en write() */
        fprintf(stderr, "✗ Error al escribir: ");
        perror("write");
        printf("\n");
    } else {
        printf("✓ Escritura exitosa: %ld bytes escritos\n\n", bytes_written);
        
        /*
         * IMPORTANTE: Si queremos leer después de escribir,
         * necesitamos reposicionar el puntero al inicio del fichero
         */
        if (lseek(fd, 0, SEEK_SET) == -1) {
            fprintf(stderr, "Error al reposicionar el puntero: ");
            perror("lseek");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    
    /*
     * INTENTAR LEER DEL FICHERO
     * 
     * read() lee datos del descriptor de fichero
     * Si el fichero se abrió con O_WRONLY (solo escritura),
     * read() fallará con error EBADF (Bad file descriptor)
     */
    printf("Intentando LEER del fichero...\n");
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read == -1) {
        /* Error en read() */
        fprintf(stderr, "✗ Error al leer: ");
        perror("read");
        printf("\n");
    } else if (bytes_read == 0) {
        printf("✓ Lectura exitosa, pero el fichero está vacío\n\n");
    } else {
        /* Lectura exitosa */
        buffer[bytes_read] = '\0';  /* Añadir terminador null string */
        printf("✓ Lectura exitosa: %ld bytes leídos\n", bytes_read);
        printf("Contenido:\n%s\n", buffer);
        printf("\n");
    }
    
    /*
     * CERRAR EL FICHERO
     * 
     * close() cierra el descriptor de fichero y libera recursos
     * Es importante siempre cerrar los ficheros abiertos
     */
    if (close(fd) == -1) {
        fprintf(stderr, "Error al cerrar el fichero: ");
        perror("close");
        exit(EXIT_FAILURE);
    }
    
    printf("========================================\n");
    printf("✓ Programa finalizado correctamente\n");
    printf("========================================\n");
    
    return EXIT_SUCCESS;
}
/*
ejec : gcc -Wall -Wextra apertura.c -o apertura.x
echo "hola" > fichero.txt
chmod 664 fichero.txt      # rw-rw-r--
ls -l fichero.txt
/apertura.x -f fichero.txt -r
./apertura.x -f fichero.txt -w
./apertura.x -f fichero.txt -rw

*/

/*
 * ================================================================================
 * RESUMEN DE CONCEPTOS CLAVE
 * ================================================================================
 * 
 * 1. PERMISOS DE FICHERO (statically stored en i-nodo):
 *    - Están almacenados en el i-nodo del fichero
 *    - Determinan QUÉ puede hacer el usuario sobre el fichero
 *    - Se asignan con chmod
 *    - Son independientes del modo de apertura
 * 
 * 2. MODOS DE APERTURA (dynamically set por open()):
 *    - Son restricciones de acceso establecidas en la tabla interna del SO
 *    - Determinan QUÉ puede hacer EL PROCESO sobre el fichero
 *    - Se especifican con flags en open()
 *    - O_RDONLY:  solo lectura (read OK, write FALLA)
 *    - O_WRONLY:  solo escritura (write OK, read FALLA)
 *    - O_RDWR:    lectura y escritura (ambas OK)
 * 
 * 3. INTERACCIÓN ENTRE PERMISOS Y MODOS:
 *    - El SO comprueba PRIMERO los permisos del fichero
 *    - Luego comprueba el modo de apertura del proceso
 *    - AMBOS deben permitir la operación para que funcione
 * 
 * 4. FLAGS COMUNES DE open():
 *    - O_CREAT:   crea el fichero si no existe
 *    - O_EXCL:    falla si el fichero ya existe (usar con O_CREAT)
 *    - O_TRUNC:   trunca el fichero (borra contenido)
 *    - O_APPEND:  añade al final del fichero
 * 
 * 5. ERRORES COMUNES:
 *    - EBADF (Bad file descriptor): operación no permitida por modo apertura
 *    - EACCES (Permission denied): operación no permitida por permisos fichero
 *    - ENOENT (No such file or directory): fichero no existe
 *    - EISDIR (Is a directory): operación sobre un directorio
 */
