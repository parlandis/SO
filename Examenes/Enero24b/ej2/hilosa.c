#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    int fd1,fd2,i,pos;
    char c;
    char buffer[6];

    fd1 = open("output.txt", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
   // write(fd1, "00000", 5);
    pid_t hijo;
    for (i=0; i < 10; i++) {
        

        hijo= fork();
        if (hijo== 0) {
            /* Child */
            if
            sprintf(buffer, "%d", i*11111);
            fd2 = open("output.txt", O_WRONLY);
            pos = lseek(fd2, i *5, SEEK_SET);
            lseek(fd2, pos, SEEK_SET);
            write(fd2, buffer, 5);
            close(fd2);
            exit(0);
        } else {
            /* Parent */
            
            lseek(fd1, 5, SEEK_CUR);
        }
    }

	//wait for all childs to finish
    while (wait(NULL) != -1);

    lseek(fd1, 0, SEEK_SET);
    printf("File contents are:\n");
    while (read(fd1, &c, 1) > 0)
        printf("%c", (char) c);
    printf("\n");
    close(fd1);
    exit(0);
}