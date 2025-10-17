#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// Глобальные переменные
int *array;
int array_size;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int num_threads;
    int phase;
} ThreadData;

// Функция сравнения и обмена
void compareSwap(int i, int j) {
    if (array[i] > array[j]) {
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// Функция, выполняемая потоком (одна фаза)
void* threadWork(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int tid = data->thread_id;
    int num_threads = data->num_threads;
    int offset = (data->phase % 2 == 0) ? 0 : 1;
    
    // Распределяем пары между потоками
    for (int i = offset + tid * 2; i + 1 < array_size; i += num_threads * 2) {
        compareSwap(i, i + 1);
    }
    
    free(data);
    return NULL;
}

// Многопоточная сортировка
void batcherSortParallel(int num_threads) {
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    
    // Выполняем фазы последовательно
    for (int phase = 0; phase < array_size; phase++) {
        // Создаем потоки для этой фазы
        for (int t = 0; t < num_threads; t++) {
            ThreadData* data = malloc(sizeof(ThreadData));
            data->thread_id = t;
            data->num_threads = num_threads;
            data->phase = phase;
            pthread_create(&threads[t], NULL, threadWork, data);
        }
        
        // Ждем завершения фазы
        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }
    }
    
    free(threads);
}

// Однопоточная версия
void batcherSortSequential() {
    for (int phase = 0; phase < array_size; phase++) {
        int offset = (phase % 2 == 0) ? 0 : 1;
        for (int i = offset; i + 1 < array_size; i += 2) {
            compareSwap(i, i + 1);
        }
    }
}

// Проверка сортировки
int isSorted() {
    for (int i = 0; i < array_size - 1; i++)
        if (array[i] > array[i + 1])
            return 0;
    return 1;
}

// Вывод массива
void printArray() {
    int print_limit = (array_size > 20) ? 20 : array_size;
    
    if (array_size > 20)
        printf("Первые 20 элементов: ");

    printf("[");
    for (int i = 0; i < print_limit; i++) {
        printf("%d", array[i]);
        if (i < print_limit - 1) 
            printf(", ");
    }
    printf("]\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Использование: %s <размер_массива> <количество_потоков>\n", argv[0]);
        printf("Пример: %s 1000 4\n", argv[0]);
        return 1;
    }
    
    array_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    if (array_size <= 0 || num_threads <= 0) {
        printf("Ошибка: размер и потоки должны быть > 0\n");
        return 1;
    }
    
    // Создание массивов
    array = malloc(array_size * sizeof(int));
    int *array_copy = malloc(array_size * sizeof(int));
    srand(time(NULL));
    
    // Заполнение
    for (int i = 0; i < array_size; i++) {
        array[i] = rand() % 1000;
        array_copy[i] = array[i];
    }
    
    printf("\n=== Сортировка Бетчера ===\n");
    printf("Размер: %d\n", array_size);
    printf("Потоки: %d\n", num_threads);
    printf("PID: %d\n", getpid());
    
    if (array_size <= 20) {
        printf("\nДо сортировки:\n");
        printArray();
    }
    
    // Многопоточная версия
    printf("\n--- Многопоточная ---\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    batcherSortParallel(num_threads);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double parallel_time = (end.tv_sec - start.tv_sec) + 
                           (end.tv_nsec - start.tv_nsec) / 1e9;
    
    if (array_size <= 20) {
        printf("После:\n");
        printArray();
    }
    
    printf("%s\n", isSorted() ? "✓ Отсортировано" : "✗ ОШИБКА!");
    printf("Время: %.6f сек\n", parallel_time);
    
    // Однопоточная версия
    printf("\n--- Однопоточная ---\n");
    for (int i = 0; i < array_size; i++)
        array[i] = array_copy[i];
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    batcherSortSequential();
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double sequential_time = (end.tv_sec - start.tv_sec) + 
                             (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Время: %.6f сек\n", sequential_time);
    
    // Метрики
    double speedup = sequential_time / parallel_time;
    double efficiency = (speedup / num_threads) * 100.0;
    
    printf("\n--- Результаты ---\n");
    printf("Ускорение: %.2fx\n", speedup);
    printf("Эффективность: %.1f%%\n", efficiency);
    
    if (speedup < 1.0) {
        printf("\n⚠️  Многопоточность не дала выигрыша\n");
        printf("Причины:\n");
        printf("- Накладные расходы на создание/уничтожение потоков\n");
        printf("- Алгоритм O(n²) неэффективен для больших массивов\n");
        printf("- Четно-нечетная сортировка требует много синхронизации\n");
    }
    
    printf("\n💡 Просмотр потоков: ps -T -p %d\n", getpid());
    
    free(array);
    free(array_copy);
    return 0;
}