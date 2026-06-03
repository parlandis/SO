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
        fprintf(stderr, "Uso: %s <fichero_entrada>\n", argv[0]); // Mensaje de uso por stderr
        return EXIT_FAILURE;                                    // Salir indicando error
    }

    if((fp = fopen(argv[1], "rb")) == NULL){
        perror("Error al abrir el archivo\n");  // Imprime mensaje de error del sistema
        return EXIT_FAILURE;                    // Termina el programa con fallo
    }

    int id;             // Identificador entero
    double value;       // Un valor de punto flotante
    char label[LABEL_MAX_LEN + 1]; // Etiqueta de texto (hasta 16, incluyendo '\0')

    SimpleRecord rs; 

    int i = 0; 
    while(i < 3){
        if(fread(&rs, sizeof(SimpleRecord), 1, fp) != 1){
            perror("Error al leer la estructura\n");
           // fclose(fp);
            break;
        }
        else {
            printf("ID: %d, Valor: %lf, Etiqueta: %s\n", rs.id, rs.value, rs.label);
        }
        i++;
    }

    fclose(fp);
    return 0;
}