#include <stdio.h>      // Declaraciones de FILE, fopen, fprintf, fclose, perror...
#include <stdlib.h>     // Declaraciones de EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // Funciones de cadenas
#include <err.h>        // Funciones de manejo de errores
#include <unistd.h>     // Para getopt

#define LABEL_MAX_LEN 15 // Máximo número de caracteres (sin incluir '\0')

typedef struct {
    int id;                         // Identificador entero
    double value;                   // Un valor de punto flotante
    char label[LABEL_MAX_LEN + 1];  // Etiqueta de texto (hasta 16, incluyendo '\0')
} SimpleRecord;

// REGLA 1: Función para procesar cuando la ENTRADA es TEXTO
void procesar_desde_texto(FILE *in, FILE *out, char Tout) {
    SimpleRecord reg;

    // Leemos usando fscanf campo por campo (Conversión de texto a datos de computador)
    while (fscanf(in, "%d %lf %15s", &reg.id, &reg.value, reg.label) == 3) {
        if (Tout == 't') {
            // Salida Texto: Formateamos con fprintf
            fprintf(out, "%d %lf %s\n", reg.id, reg.value, reg.label);
        } else {
            // Salida Binaria: Volcamos el struct completo a bytes con fwrite
            fwrite(&reg, sizeof(SimpleRecord), 1, out);
        }
    }
}

// REGLA 2: Función para procesar cuando la ENTRADA es BINARIA
void procesar_desde_binario(FILE *in, FILE *out, char Tout) {
    SimpleRecord reg;

    // Leemos usando fread bloque por bloque de bytes
    while (fread(&reg, sizeof(SimpleRecord), 1, in) == 1) {
        if (Tout == 't') {
            // Salida Texto: Convertimos los bytes del computador a texto legible
            fprintf(out, "%d %lf %s\n", reg.id, reg.value, reg.label);
        } else {
            // Salida Binaria: Copia directa de bytes de un archivo a otro
            fwrite(&reg, sizeof(SimpleRecord), 1, out);
        }
    }
}

int main(int argc, char* argv[]) {
    FILE *in = NULL;
    FILE *out = NULL;
    
    // El enunciado dice que por defecto se asume texto ('t')
    char Tin = 't';  
    char Tout = 't';  

    // 1. Parsing de argumentos con getopt
    int opt;
    while ((opt = getopt(argc, argv, "i:o:")) != -1) {
        switch (opt) {
            case 'i':
                if (*optarg != 't' && *optarg != 'b') {
                    fprintf(stderr, "Error: El formato de entrada debe ser 't' o 'b'\n");
                    return EXIT_FAILURE;
                }
                Tin = *optarg;
                break;
            case 'o':
                if (*optarg != 't' && *optarg != 'b') {
                    fprintf(stderr, "Error: El formato de salida debe ser 't' o 'b'\n");
                    return EXIT_FAILURE;
                }
                Tout = *optarg;
                break;
            default:
                fprintf(stderr, "Uso: %s [-i t|b] [-o t|b] fichero_entrada fichero_salida\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // 2. Control de errores posicionales (Deben quedar exactamente 2 argumentos: entrada y salida)
    if (argc - optind != 2) {
        fprintf(stderr, "Error: Faltan o sobran archivos de entrada/salida.\n");
        fprintf(stderr, "Uso: %s [-i t|b] [-o t|b] fichero_entrada fichero_salida\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *fichero_in = argv[optind];
    char *fichero_out = argv[optind + 1];

    // 3. Apertura del archivo de entrada
    if (Tin == 't') {
        in = fopen(fichero_in, "r");
    } else {
        in = fopen(fichero_in, "rb");
    }
    
    if (in == NULL) {
        perror("Error al abrir el archivo de entrada");
        return EXIT_FAILURE;
    }

    // 4. Apertura del archivo de salida
    if (Tout == 't') {
        out = fopen(fichero_out, "w");
    } else {
        out = fopen(fichero_out, "wb");
    }
    
    if (out == NULL) {
        perror("Error al abrir el archivo de salida");
        fclose(in); // No dejamos descriptores abiertos
        return EXIT_FAILURE;
    }

    // 5. Bifurcación según el tipo de entrada y procesamiento
    if (Tin == 't') {
        procesar_desde_texto(in, out, Tout);
    } else {
        procesar_desde_binario(in, out, Tout);
    }

    // 6. Cierre de ficheros e indicaciones de éxito
    fclose(in);
    fclose(out);
    return EXIT_SUCCESS;
}


/*
 * fscanf()
 * - Sintaxis: int fscanf(FILE *stream, const char *format, ...);
 * - Uso: Lee datos formateados desde el flujo o archivo especificado en el primer parámetro (en lugar del teclado) y los almacena en las variables proporcionadas.
 * - En este caso, al igual que scanf, exige pasar siempre la DIRECCIÓN de memoria de las variables usando el operador '&' (excepto en arrays/cadenas).
 * - Ejemplo típico: fscanf(archivo, "%d %s", &id, nombre); // Lee un entero y una palabra separados por espacio/tabulador/salto de línea desde el archivo.
 * - Valor devuelto: El número de elementos leídos y asignados con éxito. Devuelve 0 si los datos del archivo no coinciden con el formato, o EOF (-1) si se alcanza el final del archivo (End Of File) o hay un error de lectura.
 *
 * Limitador de anchura en formato de cadenas (%15s)
 * - Uso: Especifica el número máximo de caracteres que fscanf o scanf pueden leer para una cadena de texto.
 * - En este caso: %15s asegura que nunca se lean más de 15 caracteres del archivo, protegiendo un buffer de tamaño 16 (15 caracteres + '\0').
 * - Importancia en SO: Evita vulnerabilidades de seguridad y caídas del programa por desbordamiento de búfer (Buffer Overflow).
 * - Regla de sintaxis: Las variables de texto (arrays de char) pasadas a fscanf NO llevan el operador '&' delante, ya que su propio nombre es una dirección de memoria.
 *
 * fread()
 * - Sintaxis: size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 * - Uso: Lee bloques de bytes puros de un archivo binario directamente a la memoria RAM para reconstruir estructuras (structs) o arrays.
 * - En este caso: fread(&registro, sizeof(SimpleRecord), 1, fp) copia del disco el tamaño exacto del struct y rellena todos sus campos automáticamente.
 * - Valor devuelto: El número de elementos/estructuras leídos con éxito. Devuelve 0 al llegar al final del archivo (EOF) o si hay un error.
 *
 * fwrite()
 * - Sintaxis: size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
 * - Uso: Vuelca bloques de datos binarios completos directamente de la memoria RAM al disco sin convertirlos a texto legible.
 * - En este caso: fwrite(&registro, sizeof(SimpleRecord), 1, fp) escribe los bytes de la estructura tal y como están en el computador.
 * - Valor devuelto: Cantidad de bloques/estructuras escritas con éxito (debe coincidir con nmemb).
 *
 * getopt()
 * - Sintaxis: int getopt(int argc, char *const argv[], const char *optstring);
 * - Uso: Analiza de forma iterativa las opciones y banderas pasadas por línea de comandos (argumentos que empiezan por '-').
 * - La cadena 'optstring': Lista de caracteres permitidos. Si un carácter va seguido de dos puntos (ej: "i:"), significa que esa opción requiere obligatoriamente un argumento adicional.
 * - Variables globales críticas: 
 * - optarg: Puntero al texto del argumento que acompaña a la bandera (ej: si pasas "-i t", optarg apunta a "t").
 * - optind: Índice en argv del siguiente argumento a procesar. Al terminar el bucle, apunta al primer argumento posicional (como los ficheros).
 * - Valor devuelto: El carácter de la opción encontrada en cada vuelta, o -1 cuando ya no quedan más banderas que procesar. Si encuentra un error, devuelve '?'.
 *
 * strncpy()
 * - Sintaxis: char *strncpy(char *dest, const char *src, size_t n);
 * - Uso: Copia como máximo 'n' caracteres de la cadena origen (src) en el destino (dest).
 * - Importancia en SO: Es la alternativa segura a strcpy(). Evita desbordamientos de búfer al forzar un límite estricto de bytes a copiar.
 * - Alerta de uso: Si la cadena src mide igual o más que 'n', strncpy NO añade el carácter nulo ('\0') al final automáticamente. Hay que ponerlo a mano en dest[n-1].
 *
 * perror()
 * - Sintaxis: void perror(const char *s);
 * - Uso: Imprime un mensaje de error en la salida de errores estándar (stderr) describiendo el último fallo ocurrido en una llamada al sistema.
 * - Comportamiento: Toma la cadena 's' que tú le pases, le añade dos puntos y un espacio, y concatena automáticamente la traducción del código de error global 'errno' (ej: "Error al abrir: No such file or directory").
 */