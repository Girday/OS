#include <stdio.h>
#include <stdlib.h>

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
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    FILE *f = fopen(filename, "a");
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
