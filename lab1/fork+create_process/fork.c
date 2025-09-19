#include "stdio.h"
#include "stdlib.h" // pid_t declaration is here
#include "unistd.h" // fork() and getpid() declarations are here

#define RED_COLOR    "\x1b[31m" // child process output color
#define GREEN_COLOR  "\x1b[32m" // parent process output color

int main() {
    printf(GREEN_COLOR);
    printf("Hello, World! Let's try to create a child process. My pid is '%d'\n", getpid()); //pid - process identifier
    int value = -1; // some process variable
    pid_t pid = fork(); // fork creates a new(child) process
    if (-1 == pid)
    {
        // error handling
        perror("fork");
        return -1;
    }
    else if (0 == pid)
    {
        // child process code
        printf(RED_COLOR);
        printf("Hello! I'm a child process. My value is '%d'. My pid is '%d'\n", value, getpid());
        // let's try to change 'value'
        value = 0;
    }
    else
    {
        // parent process code
        printf("I've created the child process with pid: '%d'. My value is '%d'. My pid is '%d'\n", pid, value, getpid());
        value = 1;
    }
    // child and parent processes code
    printf("My value is '%d'. My pid is '%d'\n", value, getpid());
    return 0;
}
