#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"

#define _R_            "\x1b[31m" // red color for child process output color
#define _G_            "\x1b[32m" // green color for parent process output color
#define RESET_COLOR    "\x1b[0m"  // reset color
int create_process();

int main() {
    int pipe_fd[2]; // file descriptors for pipe. pipe_fd[0] - for reading, pipe_fd[1] - for writing
    int err = pipe(pipe_fd); //pipe creating
    if (err == -1) // error handling. You can use short form: if (pipe(fd) == -1) and don't create err variable
    {
        perror("pipe");
        return -1;
    }
    pid_t pid = create_process();
    if (0 == pid)
    {
        printf(_R_"Hello! I'm a child process. I need to receive 'a' and 'b' from parent process and calculate sum\n");
        int child_a, child_b;
        close(pipe_fd[1]); // Close unused descriptors. It is important.
        read(/*descriptor to read - source*/ pipe_fd[0], /* destination buffer */ &child_a, /* requested count bytes */ sizeof(child_a)); // read 'a'
        read(pipe_fd[0], &child_b, sizeof(child_b)); // read 'b'
        int sum = child_a + child_b;
        printf(_R_"Answer is: %d\n", sum);
        close(pipe_fd[0]);
    }
    else
    {
        int a = 40, b = 2;
        printf(_G_"Hello! I'm a parent process. I need sending 'a':'%d' and 'b':'%d' to the child process\n", a, b);
        close(pipe_fd[0]); // closing the descriptor to read because parent process is not using it
        write(/* descriptor to write - destination */ pipe_fd[1], /* source buffer */ &a, /* buffer size */ sizeof(a)); // send 'a' to the child process
        write(pipe_fd[1], &b, sizeof(b)); // send 'b' to the child process
        close(pipe_fd[1]); // closing the descriptor to read because it is no longer needed 
        printf(_G_"I've sent 'a' and 'b'. Result waiting... \n");
        wait(NULL); // child process waiting. wait() is declared in sys/wait.h
        printf(_G_"The child process done a work and finished. Goodbye!\n");
    }
    printf(RESET_COLOR);
    return 0;
}

int create_process() {
    pid_t pid = fork();
    if (-1 == pid)
    {
        perror("fork");
        exit(-1);
    }
    return pid;
}
