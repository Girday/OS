#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"

#define FILE_NAME_SIZE 256
#define CHILD 0


pid_t createProcess() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(-1);
    }
    return pid;
}

int main() {    
    int pipe_id[2];

    if (pipe(pipe_id) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    printf("Please, enter the NAME of the FILE: ");
    ffllush(stdout);
    
    char file_name[FILE_NAME_SIZE];

    if (scanf("%s", file_name) != 1) {
        perror("scanf");
        return EXIT_FAILURE;
    }

    pid_t pid = createProcess();

    if (pid == CHILD) {
        close(pipe_id[1]);
        if (dup2(pipe_id[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipe_id[0]);

        execl("./child.out", "child.out", file_name, NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    } 
    else {
        printf("Enter numbers (one per line):\n");
        fflush(stdout);

        close(pipe_id[0]);
        if (dup2(pipe_id[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return EXIT_FAILURE;
        }
        close(pipe_id[1]);

        int num;
        while (scanf("%d", &num) == 1) {
            printf("%d\n", num);
            fflush(stdout);
        }

        close(STDOUT_FILENO);

        wait(NULL);
    }

    return 0;
}
