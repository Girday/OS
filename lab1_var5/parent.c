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
        exit(EXIT_FAILURE);
    }
    return pid;
}

int main() {    
    int pipe_in[2];
    int pipe_out[2];

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    printf("Please, enter the NAME of the FILE: ");
    fflush(stdout);
    
    char file_name[FILE_NAME_SIZE];
    if (scanf("%s", file_name) != 1) {
        perror("scanf");
        return EXIT_FAILURE;
    }

    pid_t pid = createProcess();

    if (pid == CHILD) {
        close(pipe_in[1]);
        close(pipe_out[0]);

        if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
            perror("dup2 stdin");
            _exit(EXIT_FAILURE);
        }
        close(pipe_in[0]);

        if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            _exit(EXIT_FAILURE);
        }
        close(pipe_out[1]);

        execl("./child.out", "child.out", file_name, NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    } 
    else {
        close(pipe_in[0]);
        close(pipe_out[1]);

        printf("Enter numbers (one per line):\n");
        fflush(stdout);

        int num;
        int signal;
        fd_set readfds;

        while (1) {
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            FD_SET(pipe_out[0], &readfds);

            int maxfd = (STDIN_FILENO > pipe_out[0]) ? STDIN_FILENO : pipe_out[0];
            select(maxfd + 1, &readfds, NULL, NULL, NULL);

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                if (scanf("%d", &num) == 1) {
                    dprintf(pipe_in[1], "%d\n", num);
                } else
                    break;
            }

            if (FD_ISSET(pipe_out[0], &readfds)) {
                int r = read(pipe_out[0], &signal, sizeof(signal));
                
                if (r <= 0) 
                    break;
                
                if (signal == 1) {
                    printf("Child sent FINISH signal\n");
                    break;
                }
            }
        }

        close(pipe_in[1]);
        close(pipe_out[0]);
        wait(NULL);
    }

    return 0;
}
