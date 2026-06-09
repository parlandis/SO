#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // Para sleep()
#include <string.h>


#define MAXPROC 16

typedef struct task {
    int valid;          // booleano, indica si la entrada es válida (1) o no (0)
    int next[MAXPROC]; // array de booleanos para sucesores: next[i]=1 si sale una arista hacia el nodo i
    int id;            // identificador/nombre de la tarea, que corresponde a su posición en el array de nodos
}task_t;

typedef struct {
    int id;
    int prev[MAXPROC];   // previos 
}taskInv_t;

void print_graph(struct task tasks[], int n)
{
    int i,j;
    for (i = 0; i < n; i++) {
        if (!tasks[i].valid)
        continue;
        printf("%d: ", tasks[i].id);
        for (j = 0; j < n; j++)
            if (tasks[i].next[j])
                printf("%d ", j);
            printf("\n");
    }
}


int main(int argc, char *argv[]){

    if(argc < 2){
        fprintf(stderr, "Usage: %s archivo de entrada \n", argv[0]);
        return EXIT_FAILURE;
    }  
    printf("1");
    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL){
        fprintf(stderr, "Error al abir el fichero de entrada\n");
        return EXIT_FAILURE;
    }

    task_t tasks[MAXPROC];

    char linea[256];

    while(fgets(linea, sizeof(linea), fp) != NULL){
        if(linea[strlen(linea) - 1] == '\n'){
                linea[strlen(linea) - 1] = '\0';
        }   
    
        taskInv_t *taskP = malloc(sizeof(taskInv_t));
        //procesamos lo que hemos conseguido
        if(strlen(linea) < 3){
            fprintf(stderr, "Tiene que haber minimo dos numeros\n");
            return EXIT_FAILURE;
        }

        char c; 
        int cont = 0; 
        taskP->id = linea[cont];
        printf("Nodo: %d", taskP->id);
        cont += 2;
        while(cont < strlen(linea)){

        }
        


    }






}