#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FINISH 0
#define WRITE 1

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

    int num;
    while (scanf("%d", &num) == 1) {
        if (isPrime(num) == FINISH)
            break;

        char buffer[32];
        int len = snprintf(buffer, sizeof(buffer), "%d\n", num);

        if (write(f, buffer, len) == -1) {
            perror("write");
            close(f);
            return EXIT_FAILURE;
        }
    }

    close(f);
    return 0;
}
