#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h> 

int fd;



void *hilo(void *arg){
    int *i = (int * ) arg;
    
    fd = open("output.txt", O_RDWR, S_IRUSR | S_IWUSR);
    int n = *i *1;
    char buffer[6];
    sprintf(buffer, "%d%d%d%d%d", n,n,n,n,n);
    int pos = lseek(1, *i*5, SEEK_SET);
    write(1, buffer, 5);
}

int main(void)
{
    int fd1,fd2,i,pos;
    char c;
    char buffer[6];

    fd1 = open("output.txt", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);

    pthread_t hilos[5];

    for(i = 0; i < 10; i++){
        int *j = malloc(sizeof(int));
        *j = i;
       if(pthread_create(&hilos[i], NULL , hilo, j) != 0){

        	fprintf(stderr, "Error al crear el hilo %d", *j);
			return EXIT_FAILURE;
       }

    }

	for(int i = 0; i < 10; i++){
		pthread_join(hilos[i], NULL);
	}
    printf("\n");

    exit(0);
}