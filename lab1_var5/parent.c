#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/select.h"
#include "sys/wait.h"

#define FILE_NAME_SIZE 256
#define CHILD 0
#define FINISH 1


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
        fd_set readfds; // множество дескрипторов для select (на чтение):
                        // там всё типа 01001010010, где 1 значит, 
                        //     что i-й дескриптор участвует в операции

        while (1) {
            FD_ZERO(&readfds); // конкретное обнуление множества

            FD_SET(STDIN_FILENO, &readfds); // какой-то 0 заменяется на 1,
            FD_SET(pipe_out[0], &readfds);  // и этот дескриптор теперь участвует в операции

            int maxfd = (STDIN_FILENO > pipe_out[0]) ? STDIN_FILENO : pipe_out[0];
            // максимальный дескриптор из всех, что мы слушаем
           
            select(maxfd + 1, &readfds, NULL, NULL, NULL);
            /* 
            select(максимальный файловый дескриптор + 1, 
                   множество файловых дескрипторов на чтение,
                   множество файловых дескрипторов на запись,
                   множество файловых дескрипторов на исключения,
                   время ожидания (NULL - ждать бесконечно));
            
            После select в readfds будут стоять 1 только у тех дескрипторов, 
            которые готовы к чтению, и 0, если не готовы
            */

            if (FD_ISSET(STDIN_FILENO, &readfds)) { // если в множестве на чтение в stdin стоит 1
                if (scanf("%d", &num) == 1)
                    dprintf(pipe_in[1], "%d\n", num);
                else
                    break;
            }

            if (FD_ISSET(pipe_out[0], &readfds)) { // если в множестве на чтение в пайп стоит 1
                if (read(pipe_out[0], &signal, sizeof(signal)) <= 0) 
                    break;
                
                if (signal == FINISH) {
                    printf("FINISH child -> parent\n");
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
