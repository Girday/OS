#include <stdio.h>
#include <dlfcn.h>
#include <math.h>

double (*SinIntegral)(double, double, double) = NULL;
double (*E)(double) = NULL;

void *current_lib = NULL;
int current_implementation = 1;

int load_library(int implementation) {
    if (current_lib != NULL) {
        dlclose(current_lib);
        current_lib = NULL;
    }
    
    const char *lib_path;
    if (implementation == 1)
        lib_path = "../my_libs/lib1.so";
    else
        lib_path = "../my_libs/lib2.so";
    
    current_lib = dlopen(lib_path, RTLD_LAZY);
    if (!current_lib) {
        fprintf(stderr, "Ошибка загрузки библиотеки: %s\n", dlerror());
        return 0;
    }
    
    SinIntegral = (double (*)(double, double, double))dlsym(current_lib, "SinIntegral");
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Ошибка загрузки функции SinIntegral: %s\n", error);
        dlclose(current_lib);
        current_lib = NULL;
        return 0;
    }
    
    E = (double (*)(double))dlsym(current_lib, "E");
    error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Ошибка загрузки функции E: %s\n", error);
        dlclose(current_lib);
        current_lib = NULL;
        return 0;
    }
    
    current_implementation = implementation;
    printf("Загружена библиотека: lib%d.so\n\n", implementation);
    return 1;
}

void demonstration() {
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
    printf("  0       - переключить реализацию библиотеки\n");
    printf("  1 A B e - вычислить SinIntegral(A, B, e)\n");
    printf("  2 x     - вычислить E(x)\n");
    printf("  3       - выход\n\n");

    int choice;

    while (1) {
        printf("Введите команду: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода!\n");
            while (getchar() != '\n');
            continue;
        }
        
        if (choice == 3) {
            printf("Выход из программы.\n");
            break;
        }
        else if (choice == 0) {
            int new_impl = (current_implementation == 1) ? 2 : 1;
            if (load_library(new_impl))
                printf("Переключено на реализацию %d\n\n", new_impl);
            else
                printf("Ошибка при переключении библиотеки!\n\n");
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
            printf("SinIntegral(%.4f, %.4f, %.4f) = %.10f\n", A, B, e, result);
            printf("(реализация %d)\n\n", current_implementation);
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
            printf("E(%.4f) = %.10f\n", x, result);
            printf("(реализация %d)\n\n", current_implementation);
        }
        else {
            printf("Неизвестная команда. Используйте 0, 1, 2 или 3.\n");
            while (getchar() != '\n');
        }
    }
}

int main() {    
    if (!load_library(1)) {
        fprintf(stderr, "Не удалось загрузить библиотеку.\n");
        return 1;
    }
    
    demonstration();
    console();
    
    if (current_lib != NULL)
        dlclose(current_lib);
    
    return 0;
}
