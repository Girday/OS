#include "../include/isPrime.h"

#define TRUE 1
#define FALSE 0

int isPrime(int n) {
    if (n <= 1) 
        return FALSE;
    for (int i = 2; i * i <= n; i++) 
        if (n % i == 0) 
            return FALSE;
    return TRUE;
}
