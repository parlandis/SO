#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h> 

int fd1;

pthread_mutex_t mutex;
pthread_cond_t condHilo;

int turno;

void *hilo(void *arg){
    int *i = (int * ) arg;

    pthread_mutex_lock(&mutex);

    while(turno != *i){  //Si no es su turno no puede escirbir ni coger el fd
        pthread_cond_wait(&condHilo, &mutex);
    }

    //Aqui sabemos que es su turno asique puede escribir 
    int n = *i *1;
    char buffer[6];
    sprintf(buffer, "%d%d%d%d%d", n,n,n,n,n);
    int pos = lseek(fd1, *i*5, SEEK_SET);
    write(fd1, buffer, 5);

    turno++;
    pthread_cond_broadcast(&condHilo); // Despertamos a todos, ya que solo entrara al que le toque 
    pthread_mutex_unlock(&mutex);
}

int main(void)
{
    int fd2,i,pos;
    char c;
    char buffer[6];

    fd1 = open("output.txt", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);

    pthread_t hilos[5];
    turno = 0;

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



    lseek(fd1, 0, SEEK_SET);
    printf("File contents are:\n");
    while (read(fd1, &c, 1) > 0)
        printf("%c", (char) c);
    printf("\n");
    close(fd1);
    exit(0);
}