#include <stdio.h>      /* printf, fprintf, perror */
#include <stdlib.h>     /* exit, EXIT_SUCCESS, EXIT_FAILURE */
#include <fcntl.h>      /* open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC */
#include <unistd.h>     /* read, write, close, getopt */
#include <string.h>     /* strlen */
#include <sys/stat.h>   /* S_IRUSR, S_IWUSR, S_IRGRP, S_IROTH */
#include <errno.h>      /* errno para mensajes de error */
#include <ctype.h>
#include <sys/wait.h>   // waitpid, macros WEXITSTATUS, WNOHANG

int main(int argc, char *argv[]){
    if(argc < 3){
	    fprintf(stderr, "Uso: %s -o c <fichero1><fichero2>\n", argv[0]);
        return EXIT_FAILURE;
    }  

    int opt;
    FILE *m1, *m2;
    char op;

    while((opt = getopt(argc, argv, "o:")) != 1){
        switch (opt)
        {
        case 'o':
            op = *optarg;
            printf("Operacion : %c\n", op);
            if(op != '+'){
                fprintf(stderr, "La operacion debe ser * o +\n");
                return EXIT_FAILURE;
            }
        }
    }

    if(argc - optind != 2){
        fprintf(stderr, "Error: Debe especificar exactamente dos archivos de entrada.\n");
        fprintf(stderr, "Uso: %s -o c <fichero1><fichero2>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *in = argv[optind];
    char c = in[strlen(in) - 1];
    printf("Ultimo %c", c);

    for(int i = optind; i < argc; i++){
        int cont = 0;
        char ** args; 
        char *in = argv[i];
        args[cont] = malloc(strlen(in + 4)); // Reservamos fichero + .out

        for(int j = 0; j < strlen(in); j++){
            *args[j] = in[j];
        }
        args[strlen(in) - 1];
    }

    if((m1 = fopen(argv[optind], "r")) == NULL ){
        fprintf(stderr, "Error al abrir el primer archivo\n");
        return EXIT_FAILURE;
    }
    optind++;



    if((m2 = fopen(argv[optind], "r")) == NULL ){
        fprintf(stderr, "Error al abrir el segundo archivo\n");
        fclose(m1);
        return EXIT_FAILURE;
    }




    return 0;
}