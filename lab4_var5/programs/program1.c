#include <stdio.h>
#include <math.h>

#include "../my_libs/lib.h"

int main() {
    printf("Функция SinIntegral\n\n");
    printf("%-5s %-24s %-21s %-20s\n", "i", "Результат", "Точное", "Погрешность");
    
    for (int i = 1; i <= 10; i++) {
        double x = i;
        double result = SinIntegral(0, x, 0.0001);
        double exact = 1.0 - cos(x);
        double error = fabs(result - exact);
        
        printf("%-5d %-15.8f %-15.8f %-15.8f\n", i, result, exact, error);
    }
    
    printf("\n\nФункция E\n\n");
    printf("%-5s %-24s %-21s %-20s\n", "i", "Результат", "Точное", "Погрешность");
    
    double e_exact = exp(1);
    
    for (int i = 1; i <= 10; i++) {
        double x = i * 10;
        double result = E(x);
        double error = fabs(result - e_exact);
        
        printf("%-5d %-15.8f %-15.8f %-15.8f\n", i, result, e_exact, error);
    }
    
    return 0;
}
