#include <stdio.h>      // Declaraciones de FILE, fopen, fprintf, fclose, perror...
#include <stdlib.h>     // Declaraciones de EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // Funciones de cadenas (aquí no se usan, pero se incluye por si acaso)
#include <err.h>        // Funciones de manejo de errores (aquí tampoco se usan)

#define LABEL_MAX_LEN 15 // Máximo número de caracteres (sin incluir '\0')
typedef struct {
    int id;             // Identificador entero
    double value;       // Un valor de punto flotante
    char label[LABEL_MAX_LEN + 1]; // Etiqueta de texto (hasta 16, incluyendo '\0')
} SimpleRecord;

int main(int argc, char* argv[]){
        FILE *fp;
    if(argc != 2){
        fprintf(stderr, "Uso: %s <fichero_salida>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if((fp = fopen(argv[1], "wb")) == NULL){
        perror("Error al abrir el archivo\n");
        return EXIT_FAILURE;
    }

    SimpleRecord records[3] = {
        {1, 3.10, "Barcelona"},
        {0, 19.77, "Madrid"},
        {2, 7.42, "Valencia"}
    };


    for(int i = 0; i < 3; i++){
         /* fwrite escribe bloques de memoria tal cual en el fichero:
         *  - &records[i] : dirección del registro i-ésimo
         *  - sizeof(SimpleRecord) : tamaño de un registro en bytes
         *  - 1 : número de elementos (registros) a escribir
         *  - fp : fichero de destino
         *
         * Devuelve el número de elementos escritos (aquí, registros).
         * Si no es 1, ha habido un error de escritura.
         */

        if(fwrite(&records[i], sizeof(SimpleRecord), 1, fp) != 1){
            perror("fwrite");   // Muestra el motivo del error en la escritura
            fclose(fp);         // Cierra el fichero antes de salir
            return EXIT_FAILURE;
        }
    }


    printf("La escritura ha finalizado correctamente\n");
    fclose(fp);

    return 0;
}
/*
 * FUNCIONES CLAVE EXPLICADAS:
 *
 * fopen(const char *pathname, const char *mode)
 *  - Abre un fichero y devuelve un puntero FILE* que representa ese flujo.
 *  - Modos relevantes aquí:
 *      "wb" → abrir para escritura en binario, truncando el fichero si existe
 *             o creándolo si no existe.
 *  - Si la apertura falla (por ejemplo, por falta de permisos o ruta inexistente),
 *    devuelve NULL y es obligatorio comprobarlo antes de usar el FILE*.
 *
 * fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
 *  - Escribe datos binarios en el fichero asociado a 'stream'.
 *  - Interpreta que en la dirección 'ptr' hay 'nmemb' elementos,
 *    cada uno de tamaño 'size' bytes, y los vuelca tal cual al archivo.
 *  - En este programa:
 *      fwrite(&records[i], sizeof(SimpleRecord), 1, fp)
 *    escribe un registro completo (estructura) de golpe.
 *  - Devuelve el número de elementos escritos. Si es menor que 'nmemb'
 *    (por ejemplo, 0 o distinto de 1 en este caso), se ha producido un error de escritura.
 *
 * fclose(FILE *stream)
 *  - Cierra el fichero asociado a 'stream'.
 *  - Vacía cualquier buffer de salida pendiente (muy importante en binario y texto),
 *    libera recursos del sistema y marca el FILE* como no válido.
 *
 * fprintf(FILE *stream, const char *format, ...)
 *  - Igual que printf, pero enviando la salida al flujo indicado (aquí, stderr).
 *  - Se usa para mostrar mensajes de uso o error sin mezclarlos con la salida normal.
 *
 * perror(const char *msg)
 *  - Imprime el mensaje 'msg' seguido de la descripción del último error de sistema.
 *  - Muy útil tras llamadas fallidas como fopen o fwrite para saber por qué se ha producido el fallo.
 *
 * EXIT_SUCCESS / EXIT_FAILURE (stdlib.h)
 *  - Constantes que indican éxito o error al terminar el programa.
 *  - Permiten que la shell o scripts detecten si el programa acabó correctamente.
 *
 * En resumen:
 *  - Este programa demuestra cómo guardar un array de estructuras en un fichero BINARIO,
 *    escribiendo cada registro con fwrite.
 *  - A diferencia de fprintf (texto), aquí se vuelca la representación en memoria de la
 *    estructura, lo que permite luego leerla de vuelta con fread de forma directa.
 */