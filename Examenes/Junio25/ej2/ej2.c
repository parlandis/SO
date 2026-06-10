#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // Para sleep()
#include <pthread.h> //Libreria para trabajar con hilos
#include <fcntl.h>      // Control de archivos: define constantes para 'open' (O_CREAT, O_RDONLY, etc.).
#include <string.h>     /* strlen */

pthread_mutex_t mutex;       // El "cerrojo" para la sección crítica
pthread_cond_t condProd;      
pthread_cond_t condCons;  

int ind;
int fin;

FILE *entrada;
FILE *salida;

#define MAX_SBUFFER_SIZE 4
char* shared_buffer[MAX_SBUFFER_SIZE];   // Array de punteros a char

static inline char* clone_string(char* original)
{
	char* copy;
	copy=malloc(strlen(original)+1);
	strcpy(copy,original);
	return copy;
}

void *prodF(void *arg){
    printf("Creando productor\n");
    char linea[256];
    ind = 0; 

    // CORRECCIÓN: El fgets debe estar FUERA del mutex para no bloquear el fichero entero
    while(fgets(linea, sizeof(linea), entrada) != NULL && fin == 0){
        
        // Clonamos la cadena fuera de la sección crítica
        char *buff = clone_string(linea);    

        pthread_mutex_lock(&mutex);
        
        // Si el buffer se ha llenado (ind == 4), esperamos a que el consumidor lo vacíe
        while(ind == MAX_SBUFFER_SIZE){
            printf("Productor: shbuff lleno, despertando al consumidor\n");
            pthread_cond_signal(&condCons); 
            pthread_cond_wait(&condProd, &mutex); 
        }

        // Insertamos de forma segura porque ya sabemos que ind < 4
        printf("Productor: leyendo una linea \n");
        shared_buffer[ind] = buff;
        printf("Copiado:%s \n", shared_buffer[ind]);
        ind++;

        // Si justo acabamos de llenar la cuarta posición, avisamos al consumidor
        if(ind == MAX_SBUFFER_SIZE){
            pthread_cond_signal(&condCons);
        }

        pthread_mutex_unlock(&mutex);
    }

    // CORRECCIÓN: Al salir del bucle (Fin de Fichero), insertamos el NULL de parada que pide el enunciado
    pthread_mutex_lock(&mutex);
    while(ind == MAX_SBUFFER_SIZE){
        pthread_cond_wait(&condProd, &mutex);
    }
    shared_buffer[ind] = NULL; 
    ind++; // Avanzamos para que el consumidor sepa cuántos elementos leer
    
    printf("Productor: Fin de fichero, enviado NULL de parada.\n");
    pthread_cond_signal(&condCons); // Despertamos al consumidor para que procese el final
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *consF(void *arg){
    printf("Creando consumidor\n");
    
    while(fin == 0){
        pthread_mutex_lock(&mutex);
        
        // CORRECCIÓN: Esperamos a que haya datos (ind > 0) con el mutex bloqueado (Norma POSIX)
        while(ind == 0){
            printf("Esperando a que el buffer tenga datos o este lleno\n");
            pthread_cond_wait(&condCons, &mutex);
        }
        
        printf("Consumidor despierto, procesando buffer\n");

        // Guardamos cuántos elementos ha dejado el productor en esta tanda
        int elementos_a_procesar = ind;
        int i = 0; 

        // Recorremos los elementos que el productor ha metido
        while(i < elementos_a_procesar){
            
            if(shared_buffer[i] == NULL){
                printf("Consumidor: Detectada señal NULL de parada.\n");
                fin = 1; // Activamos la condición de fin
                break;   // Salimos del bucle de procesamiento
            }

            fputs(shared_buffer[i], salida);
            
            // CORRECCIÓN CRÍTICA: Primero liberamos el string actual, y LUEGO incrementamos 'i'
            free(shared_buffer[i]);
            i++;
        }

        // Reseteamos el índice del buffer ya que lo hemos vaciado por completo
        ind = 0;

        // Avisamos al productor de que el buffer está completamente libre y desbloqueamos
        pthread_cond_signal(&condProd);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // Mantenemos tu main exactamente como estaba, está perfecto
    if(argc < 1){
        fprintf(stderr, "Error falta el archvio de antrada\n");
        return EXIT_FAILURE;
    }

    int opt, mode_i = 0, mode_o = 0;
    char * input;
    char *output;
    entrada = stdin;
    salida = stdout;
    fin = 0; 

    while((opt = getopt(argc, argv, "hi:o:")) != -1){
        switch (opt)
        {
        case 'h':
            printf("Uso: ./pc-files [-i fichero_entrada] [-o fichero_salida] [-h]\n");
            break;
        case 'i':
            mode_i = 1;
            input = optarg;
            printf("Archivo de entrada %s\n", input);
            break;
        case 'o':
            mode_o = 1;
            output = optarg;
            printf("Archivo de salida %s\n", output);
            break;
        default:
            break;
        }
    }
    
    if(mode_i == 1){
        entrada = fopen(input, "r");
        if(entrada == NULL){
            fprintf(stderr, "Error al abrir el archivo de entrada\n");
            return EXIT_FAILURE;
        }
    }
    if(mode_o == 1){
        salida = fopen(output, "w");
        if(salida == NULL){
            fprintf(stderr, "Error al abrir el archivo de salida\n");
            if(mode_i == 1) fclose(entrada);
            return EXIT_FAILURE;
        }
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condProd, NULL);
    pthread_cond_init(&condCons, NULL);

    pthread_t prodH;
    pthread_t consH;

    if(pthread_create(&prodH, NULL, prodF, NULL) != 0){
        fprintf(stderr, "Error al crear prod\n");
        if(mode_i == 1) fclose(entrada);
        if(mode_o == 1) fclose(salida);
        return EXIT_FAILURE;
    }

    if(pthread_create(&consH, NULL, consF, NULL) != 0){
        fprintf(stderr, "Error al crear cons\n");
        if(mode_i == 1) fclose(entrada);
        if(mode_o == 1) fclose(salida);
        return EXIT_FAILURE;
    }

    pthread_join(prodH, NULL);
    pthread_join(consH, NULL);

    fclose(entrada);
    fclose(salida);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condProd);
    pthread_cond_destroy(&condCons);

    printf("Terminando\n");
    return 0;
}