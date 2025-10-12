#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

// Глобальные переменные для управления потоками
sem_t thread_limiter;
int max_threads;
int active_threads = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи аргументов в поток
typedef struct {
    int* arr;
    int low;
    int cnt;
    int dir;
} ThreadArgs;

// Функция для обмена двух элементов
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Функция сравнения и обмена
void compareExchange(int arr[], int i, int j, int dir) {
    if (dir == (arr[i] > arr[j])) {
        swap(&arr[i], &arr[j]);
    }
}

// Последовательное чётно-нечётное слияние
void batcherMergeSeq(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int m = cnt / 2;
        
        for (int i = low; i < low + cnt - m; i++) {
            compareExchange(arr, i, i + m, dir);
        }
        
        batcherMergeSeq(arr, low, m, dir);
        batcherMergeSeq(arr, low + m, cnt - m, dir);
    }
}

// Последовательная сортировка Бетчера
void batcherSortSeq(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int m = cnt / 2;
        batcherSortSeq(arr, low, m, 1);
        batcherSortSeq(arr, low + m, cnt - m, 0);
        batcherMergeSeq(arr, low, cnt, dir);
    }
}

// Поток для сортировки
void* batcherSortThread(void* args) {
    ThreadArgs* targs = (ThreadArgs*)args;
    batcherSortSeq(targs->arr, targs->low, targs->cnt, targs->dir);
    
    pthread_mutex_lock(&counter_mutex);
    active_threads--;
    pthread_mutex_unlock(&counter_mutex);
    
    sem_post(&thread_limiter);
    free(targs);
    return NULL;
}

// Многопоточная сортировка Бетчера
void batcherSortParallel(int arr[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int m = cnt / 2;
        
        // Пытаемся создать потоки для параллельной обработки
        int can_parallelize = 0;
        if (cnt > 1000) { // Порог для создания потока
            sem_wait(&thread_limiter);
            pthread_mutex_lock(&counter_mutex);
            if (active_threads < max_threads) {
                active_threads++;
                can_parallelize = 1;
            }
            pthread_mutex_unlock(&counter_mutex);
            
            if (!can_parallelize) {
                sem_post(&thread_limiter);
            }
        }
        
        if (can_parallelize) {
            pthread_t thread1;
            ThreadArgs* args1 = malloc(sizeof(ThreadArgs));
            args1->arr = arr;
            args1->low = low;
            args1->cnt = m;
            args1->dir = 1;
            
            pthread_create(&thread1, NULL, batcherSortThread, args1);
            
            // Вторую половину обрабатываем в текущем потоке
            batcherSortParallel(arr, low + m, cnt - m, 0);
            
            pthread_join(thread1, NULL);
        } else {
            // Последовательная обработка
            batcherSortSeq(arr, low, m, 1);
            batcherSortSeq(arr, low + m, cnt - m, 0);
        }
        
        batcherMergeSeq(arr, low, cnt, dir);
    }
}

// Функция для генерации случайного массива
void generateRandomArray(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 10000;
    }
}

// Функция для копирования массива
void copyArray(int dest[], int src[], int n) {
    memcpy(dest, src, n * sizeof(int));
}

// Функция для нахождения ближайшей степени двойки (больше или равной n)
int nextPowerOfTwo(int n) {
    int power = 1;
    while (power < n) {
        power *= 2;
    }
    return power;
}

// Функция для проверки отсортированности
int isSorted(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i-1]) {
            return 0;
        }
    }
    return 1;
}

// Функция для вывода массива
void printArray(int arr[], int n, int max_print) {
    int print_count = n < max_print ? n : max_print;
    for (int i = 0; i < print_count; i++) {
        printf("%d ", arr[i]);
    }
    if (n > max_print) {
        printf("... (всего %d элементов)", n);
    }
    printf("\n");
}

// Функция для замера времени
double getTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char* argv[]) {
    // Параметры по умолчанию
    int array_size = 10000;
    max_threads = 4;
    
    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) {
            if (i + 1 < argc) {
                max_threads = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--size") == 0) {
            if (i + 1 < argc) {
                array_size = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Использование: %s [опции]\n", argv[0]);
            printf("Опции:\n");
            printf("  -t, --threads <число>  Максимальное количество потоков (по умолчанию: 4)\n");
            printf("  -n, --size <число>     Размер массива (по умолчанию: 10000)\n");
            printf("  -h, --help             Показать эту справку\n");
            return 0;
        }
    }
    
    printf("=== Многопоточная сортировка Бетчера ===\n");
    printf("Размер массива: %d\n", array_size);
    printf("Максимальное количество потоков: %d\n\n", max_threads);
    
    // Инициализация семафора
    sem_init(&thread_limiter, 0, max_threads);
    
    // Выделение памяти для массивов
    int* original_array = malloc(array_size * sizeof(int));
    int* parallel_array = malloc(array_size * sizeof(int));
    int* sequential_array = malloc(array_size * sizeof(int));
    
    // Генерация случайного массива
    srand(time(NULL));
    generateRandomArray(original_array, array_size);
    
    printf("Первые 20 элементов исходного массива:\n");
    printArray(original_array, array_size, 20);
    printf("\n");
    
    // Параллельная сортировка
    copyArray(parallel_array, original_array, array_size);
    printf("Запуск параллельной сортировки...\n");
    double start_parallel = getTime();
    batcherSortParallel(parallel_array, 0, array_size, 1);
    double end_parallel = getTime();
    double time_parallel = end_parallel - start_parallel;
    
    printf("Время параллельной сортировки: %.6f сек\n", time_parallel);
    printf("Массив отсортирован: %s\n\n", isSorted(parallel_array, array_size) ? "ДА" : "НЕТ");
    
    // Последовательная сортировка
    copyArray(sequential_array, original_array, array_size);
    printf("Запуск последовательной сортировки...\n");
    double start_seq = getTime();
    batcherSortSeq(sequential_array, 0, array_size, 1);
    double end_seq = getTime();
    double time_seq = end_seq - start_seq;
    
    printf("Время последовательной сортировки: %.6f сек\n", time_seq);
    printf("Массив отсортирован: %s\n\n", isSorted(sequential_array, array_size) ? "ДА" : "НЕТ");
    
    // Вычисление метрик
    double speedup = time_seq / time_parallel;
    double efficiency = speedup / max_threads * 100;
    
    printf("=== Результаты ===\n");
    printf("Ускорение (Speedup): %.2fx\n", speedup);
    printf("Эффективность: %.2f%%\n", efficiency);
    
    printf("\nПервые 20 элементов отсортированного массива:\n");
    printArray(parallel_array, array_size, 20);
    
    // Освобождение ресурсов
    sem_destroy(&thread_limiter);
    pthread_mutex_destroy(&counter_mutex);
    free(original_array);
    free(parallel_array);
    free(sequential_array);
    
    return 0;
}