#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"

#define FILE_NAME_SIZE 256
#define CHILD 0
#define FINISH 0


pid_t createProcess() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(-1);
    }
    return pid;
}

int main() {    
    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    printf("Please, enter the NAME of the FILE: ");
    char file_name[FILE_NAME_SIZE];

    if (scanf("%s", file_name) != 1) {
        perror("scanf");
        return EXIT_FAILURE;
    }

    pid_t pid = createProcess();

    if (pid == CHILD) {

        close(pipe2[0]);        
        if (dup2(pipe2[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipe2[1]);

        close(pipe1[1]);
        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipe1[0]);

        execl("./child.out", "child.out", file_name, NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    
    } else {
    
        printf("Enter numbers (one per line):\n");
        fflush(stdout);
        


        FILE *child_stream = fdopen(pipe2[1], "r");
        if (!child_stream) {
            perror("fdopen");
            exit(1);
        }
        close(pipe2[1]);

        if (dup2(pipe1[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return EXIT_FAILURE;
        }
        close(pipe1[1]);

        int num;
        int status;
        while (scanf("%d", &num) == 1) {
            printf("%d\n", num);
            fflush(stdout);

            if (fscanf(child_stream, "%d", &status) == 1){
                printf("Status: %d\n", status);
                if (status == FINISH)
                    break;    
            } else {
                perror("fscanf");
                break;
            }
        }

        fclose(child_stream);
        close(STDOUT_FILENO);

        wait(NULL);
    }

    return 0;
}
