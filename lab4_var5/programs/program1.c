#include <stdio.h>
#include <math.h>

#include "../my_libs/lib.h"

void demostration() {
    printf("Функция SinIntegral(0, x, 0.0001)\n");
    printf("%-5s %-24s %-21s %-20s\n", "x", "Результат", "Точное", "Погрешность");
    
    for (int i = 1; i <= 10; i++) {
        double result = SinIntegral(0, i, 0.0001);
        double exact = 1.0 - cos(i);
        double error = fabs(result - exact);
        
        printf("%-5d %-15.8f %-15.8f %-15.8f\n", i, result, exact, error);
    }
    
    printf("\nФункция E(x)\n");
    printf("%-5s %-24s %-21s %-20s\n", "x", "Результат", "Точное", "Погрешность");
    
    double e_exact = exp(1);
    
    for (int i = 1; i <= 10; i++) {
        double result = E(i * 10);
        double error = fabs(result - e_exact);
        
        printf("%-5d %-15.8f %-15.8f %-15.8f\n", i * 10, result, e_exact, error);
    }
}

void console() {
    printf("\n\nИНТЕРАКТИВНЫЙ РЕЖИМ\n");
    printf("Доступные команды:\n");
    printf("  1 A B e - вычислить SinIntegral(A, B, e)\n");
    printf("  2 x     - вычислить E(x)\n");
    printf("  0       - выход\n\n");

    int choice;

    while (1) {
        printf("Введите команду: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода!\n");
            while (getchar() != '\n');
            continue;
        }
        
        if (choice == 0) {
            printf("Выход из программы.\n");
            break;
        }
        else if (choice == 1) {
            double A, B, e;
            if (scanf("%lf %lf %lf", &A, &B, &e) != 3) {
                printf("Ошибка: ожидалось 3 аргумента (A B e)\n");
                while (getchar() != '\n');
                continue;
            }
            
            if (e <= 0) {
                printf("Ошибка: шаг e должен быть положительным\n");
                continue;
            }
            
            double result = SinIntegral(A, B, e);
            printf("SinIntegral(%.4f, %.4f, %.4f) = %.10f\n\n", A, B, e, result);
        }
        else if (choice == 2) {
            double x;
            if (scanf("%lf", &x) != 1) {
                printf("Ошибка: ожидался 1 аргумент (x)\n");
                while (getchar() != '\n');
                continue;
            }
            
            if (x <= 0) {
                printf("Ошибка: x должен быть положительным\n");
                continue;
            }
            
            double result = E(x);
            printf("E(%.4f) = %.10f\n\n", x, result);
        }
        else {
            printf("Неизвестная команда. Используйте 0, 1 или 2.\n");
            while (getchar() != '\n');
        }
    }
}

int main() {    
    demostration();
    console();    
    return 0;
}
