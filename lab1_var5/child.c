#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FINISH 1
#define WRITE 0

int isPrime(int n) {
    if (n <= 1)
        return FINISH;

    for (int i = 2; i * i <= n; i++)
        if (n % i == 0)
            return WRITE;

    return FINISH;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Output file is missing\n");
        return EXIT_FAILURE;
    }

    int f = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (f == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    int num;
    while (scanf("%d", &num) == 1) {
        if (isPrime(num) == WRITE) {
            dprintf(f, "%d\n", num);
        } else {
            int signal = FINISH;
            write(STDOUT_FILENO, &signal, sizeof(signal));
            break;
        }
    }

    close(f);
    
    return 0;
}
