#include <stdio.h>
#include <stdlib.h>

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

    FILE *f = fopen(argv[1], "w");
    if (!f) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    int num;
    while (scanf("%d", &num) == 1) {
        if (isPrime(num) == WRITE) {
            fprintf(f, "%d\n", num);
            fflush(f);
        } else
            break;
    }

    fclose(f);
    return 0;
}
