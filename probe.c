#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { READ = 0, WRITE = 1 };

int main(void) {
    int pipefd[2] = {-1, -1};

    printf("Before pipe(): pipefd[0]=%d, pipefd[1]=%d\n",
           pipefd[0], pipefd[1]);

    if (-1 == pipe(pipefd)) {
        perror("pipe");
        exit(1);
    }

    printf("After  pipe(): pipefd[0]=%d, pipefd[1]=%d\n",
           pipefd[0], pipefd[1]);
    printf("  pipefd[READ]  = %d  (read end)\n",  pipefd[READ]);
    printf("  pipefd[WRITE] = %d  (write end)\n", pipefd[WRITE]);

    /* Write a message into the pipe */
    char message[] = "hello from the write end\n";
    write(pipefd[WRITE], message, strlen(message));
    close(pipefd[WRITE]);

    /* Read it back out */
    char buf[64] = {0};
    read(pipefd[READ], buf, sizeof(buf) - 1);
    close(pipefd[READ]);

    printf("Read back: %s", buf);
    printf("\n");
    printf("Notice: the same process wrote and read.\n");
    printf("In a real pipeline, two separate processes do this via fork().\n");

    return 0;
}


//   git clone https://github.com/olabamipetaiwo/csh.git