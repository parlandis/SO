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

int main(int argc, char*argv[]){
    FILE *fp;
    if(argc != 2){
        fprintf(stderr, "Uso: %s <fichero_entrada>\n", argv[0]); // Mensaje de uso por stderr
        return EXIT_FAILURE;                                    // Salir indicando error
    }

    if((fp = fopen(argv[1], "r")) == NULL){
        perror("Error al abrir el archivo\n");  // Imprime mensaje de error del sistema
        return EXIT_FAILURE;                    // Termina el programa con fallo
    }

    int id;             // Identificador entero
    double value;       // Un valor de punto flotante
    char label[LABEL_MAX_LEN + 1]; // Etiqueta de texto (hasta 16, incluyendo '\0')

    while(fscanf(fp, "%d %lf %15s", &id, &value, label) == 3){
        printf("ID: %d, Valor: %lf, Etiqueta: %s\n", id, value, label);
    }

    fclose(fp);

    return 0;
}

/*
 * fscanf()
 * - Sintaxis: int fscanf(FILE *stream, const char *format, ...);
 * - Uso: Lee datos formateados desde el flujo o archivo especificado en el primer parámetro (en lugar del teclado) y los almacena en las variables proporcionadas.
 * - En este caso, al igual que scanf, exige pasar siempre la DIRECCIÓN de memoria de las variables usando el operador '&' (excepto en arrays/cadenas).
 * - Ejemplo típico: 
 * fscanf(archivo, "%d %s", &id, nombre); // Lee un entero y una palabra separados por espacio/tabulador/salto de línea desde el archivo.
 * - Valor devuelto: El número de elementos leídos y asignados con éxito. Devuelve 0 si los datos del archivo no coinciden con el formato, o EOF (-1) si se alcanza el final del archivo (End Of File) o hay un error de lectura.
 /*
 * Limitador de anchura en formato de cadenas (%15s)
 * - Uso: Especifica el número máximo de caracteres que fscanf o scanf pueden leer para una cadena de texto.
 * - En este caso: %15s asegura que nunca se lean más de 15 caracteres del archivo, protegiendo un buffer de tamaño 16 (15 caracteres + '\0').
 * - Importancia en SO: Evita vulnerabilidades de seguridad y caídas del programa por desbordamiento de búfer (Buffer Overflow).
 * - Regla de sintaxis: Las variables de texto (arrays de char) pasadas a fscanf NO llevan el operador '&' delante, ya que su propio nombre es una dirección de memoria.
 */
 
 