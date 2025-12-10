#include "lib.h"

double SinIntegral(double A, double B, double e) {
    double sum = 0.0;
    double x = A;
    
    while (x < B) {
        sum += (sin(x) + sin(x + e)) / 2.0 * e;
        x += e;
    }

    return sum;
}

double E(double x) {
    double sum = 1.0;
    double fact = 1.0;

    for (int n = 1; n <= x; ++n) {
        fact *= n;
        sum += 1.0 / fact;
    }

    return sum;
}
