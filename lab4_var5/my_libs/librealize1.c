#include <math.h>
#include "lib.h"

double SinIntegral(double A, double B, double e) {
    double sum = 0.0;
    double x = A + e / 2.0;
    
    while (x < B) {
        sum += sin(x) * e;
        x += e;
    }

    return sum;
}

double E(double x) {
    return pow(1 + 1 / x, x);
}
