#include <stdio.h>      // Declaraciones de FILE, fopen, fprintf, fclose, perror...
#include <stdlib.h>     // Declaraciones de EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // Funciones de cadenas (aquí no se usan, pero se incluye por si acaso)
#include <err.h>        // Funciones de manejo de errores (aquí tampoco se usan)

#define LABEL_MAX_LEN 15 // Max chars sin contar el '\0'

typedef struct {
    int id;                             // Identificador entero del registro
    double value;                       // Valor numérico en coma flotante
    char label[LABEL_MAX_LEN + 1];      // Etiqueta de texto (hasta 15 chars + '\0' → 16 bytes)
} SimpleRecord;

int main(int argc, char* argv[]){

    /* Comprobar número de argumentos: se espera exactamente 1 nombre de fichero */
    if(argc != 2){
        fprintf(stderr, "Uso: %s <fichero_salida>\n", argv[0]); // Mensaje de uso por stderr
        return EXIT_FAILURE;                                    // Salir indicando error
    }

    /* Inicialización de un array de 3 registros SimpleRecord con datos de ejemplo */
    SimpleRecord records[3] = {
        {1, 3.10, "Barcelona"},
        {0, 19.77, "Madrid"},
        {2, 7.42, "Valencia"}
    };

    FILE *fp = fopen(argv[1], "w");
    if(fp == NULL){
        perror("Error al abrir el archivo\n");
		exit(EXIT_FAILURE);
    }

    for(int i = 0; i < 3; i++){
        fprintf(fp, "%d %.2f %s \n", records[i].id , records[i].value, records[i].label);
    }

    fclose(fp);
    printf("La escritura ha finalizado\n");

    return 0;
}
/*
 * FUNCIONES CLAVE EXPLICADAS:
 *
 * fprintf(FILE *stream, const char *format, ...)
 *  - Escribe texto formateado en un flujo (fichero, stdout, stderr, etc.).
 *  - Usa especificadores de formato como printf: %d (int), %f (double), %s (cadena).
 *  - En este programa se usa para volcar cada SimpleRecord como una línea de texto:
 *      fprintf(fp, "%d %.2f %s\n", id, value, label);
 *  - Devuelve un valor negativo si ocurre un error al escribir.
 *
 * fopen(const char *pathname, const char *mode)
 *  - Abre un fichero de alto nivel y devuelve un puntero FILE* asociado a él.
 *  - "w" → abre para escritura en texto, trunca el fichero si existe o lo crea si no existe.
 *  - Si falla (por ejemplo, permisos insuficientes), devuelve NULL.
 *  - Siempre se debe comprobar el valor devuelto antes de usar el FILE*.
 *
 * fclose(FILE *stream)
 *  - Cierra el fichero asociado al puntero FILE*.
 *  - Vacía los buffers pendientes de escritura y libera recursos.
 *  - Es fundamental cerrar los ficheros para que los datos se escriban correctamente en disco.
 *
 * fprintf(stderr, const char *format, ...)
 *  - Igual que fprintf, pero escribiendo por el flujo de error estándar (stderr).
 *  - Se usa para mensajes de error o de uso del programa, de forma que no se mezclan con la salida normal.
 *
 * perror(const char *msg)
 *  - Imprime el mensaje 'msg' seguido de la descripción del último error de sistema (según errno).
 *  - Muy útil tras una llamada fallida a fopen, read, write, etc., para saber el motivo del fallo.
 *
 * EXIT_SUCCESS / EXIT_FAILURE (stdlib.h)
 *  - Constantes simbólicas que indican éxito o fracaso al terminar main.
 *  - return EXIT_FAILURE; permite a la shell saber que el programa falló.
 *
 * Este programa utiliza la biblioteca estándar de C (stdio) para trabajar con ficheros de texto:
 * - Representa el fichero con FILE* en lugar de descriptores enteros.
 * - Usa fprintf para escribir registros formateados, fáciles de leer y depurar.
 * - Sigue el patrón típico: comprobar argumentos → abrir fichero → procesar datos → cerrar fichero.
 */



