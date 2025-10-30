/*
    Классическая сортировка Бетчера определена для массивов размером 2^k,
    потому что её структура — это рекурсивная сеть сравнения, 
    делящая массив пополам на каждом уровне.
    
    Однако на практике можно обрабатывать произвольные размеры, 
    если либо дополнить массив фиктивными элементами до степени двойки, 
    либо аккуратно обработать “остаток” после ближайшей степени двойки.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>

// Структура для передачи данных в поток
typedef struct {
    int* array;
    int size;
    int thread_id;
    int num_threads;
    int phase;
    pthread_barrier_t* barrier;
} thread_data_t;

// Глобальные переменные
int* g_array = NULL;
int g_array_size = 0;
int g_num_threads = 1;
pthread_barrier_t g_barrier;

// Функция обмена элементов
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Функция для проверки, является ли число степенью двойки
int is_power_of_two(int n) {
    return (n & (n - 1)) == 0 && n > 0;
}

// Функция для получения следующей степени двойки
int next_power_of_two(int n) {
    int power = 1;
    while (power < n)
        power *= 2;
    
    return power;
}

// Последовательная версия четно-нечетной сортировки Бетчера
void batcher_odd_even_sort_sequential(int* array, int size) {
    if (!is_power_of_two(size)) {
        fprintf(stderr, "Ошибка: Размер массива должен быть степенью двойки\n");
        return;
    }

    for (int p = 1; p < size; p *= 2)
        for (int k = p; k >= 1; k /= 2)
            for (int j = k % p; j < size - k; j += 2 * k)
                for (int i = 0; i < k; i++)
                    if ((i + j) / (p * 2) == (i + j + k) / (p * 2))
                        if (array[i + j] > array[i + j + k])
                            swap(&array[i + j], &array[i + j + k]);
}

// Функция, выполняемая каждым потоком
void* batcher_odd_even_sort_parallel(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int* array = data->array;
    int size = data->size;
    int thread_id = data->thread_id;
    int num_threads = data->num_threads;
    
    for (int p = 1; p < size; p *= 2) {
        for (int k = p; k >= 1; k /= 2) {
            for (int j = k % p + thread_id * (2 * k); j < size - k; j += 2 * k * num_threads)
                for (int i = 0; i < k && (i + j) < size - k; i++)
                    if ((i + j) / (p * 2) == (i + j + k) / (p * 2))
                        if (array[i + j] > array[i + j + k])
                            swap(&array[i + j], &array[i + j + k]);
            
            // Синхронизация после каждой фазы
            pthread_barrier_wait(data->barrier);
        }
    }
    
    return NULL;
}

// Генерация случайного массива
int* generate_random_array(int size) {
    int* array = (int*)malloc(size * sizeof(int));
    if (!array) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return NULL;
    }
    
    for (int i = 0; i < size; i++) {
        array[i] = rand();
    }
    
    return array;
}

// Чтение массива из файла
int* read_array_from_file(const char* filename, int* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла: %s\n", filename);
        return NULL;
    }
    
    // Подсчет количества чисел в файле
    int count = 0;
    int temp;
    while (fscanf(file, "%d", &temp) == 1) {
        count++;
    }
    
    rewind(file);
    
    int* array = (int*)malloc(count * sizeof(int));
    if (!array) {
        fclose(file);
        return NULL;
    }
    
    for (int i = 0; i < count; i++) {
        fscanf(file, "%d", &array[i]);
    }
    
    fclose(file);
    *size = count;
    return array;
}

// Проверка отсортированности массива
int is_sorted(int* array, int size) {
    for (int i = 0; i < size - 1; i++)
        if (array[i] > array[i + 1])
            return 0;

    return 1;
}

// Копирование массива
int* copy_array(int* src, int size) {
    int* dest = (int*)malloc(size * sizeof(int));
    
    if (!dest) 
        return NULL;
    
    memcpy(dest, src, size * sizeof(int));
    return dest;
}

// Вывод части массива
void print_array_partial(int* array, int size, int num_elements) {
    if (num_elements > size) num_elements = size;
    
    printf("Первые %d элементов: ", num_elements);
    for (int i = 0; i < num_elements; i++)
        printf("%d ", array[i]);

    printf("\n");
    
    printf("Последние %d элементов: ", num_elements);
    for (int i = size - num_elements; i < size; i++)
        printf("%d ", array[i]);

    printf("\n");
}

// Функция для измерения времени выполнения
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Вывод справки
void print_help() {
    printf("Использование:\n");
    printf("  ./sort -n <размер> -t <потоки> [-f <файл>]\n");
    printf("Параметры:\n");
    printf("  -n <размер>    Размер массива (по умолчанию: 1024)\n");
    printf("  -t <потоки>    Количество потоков (по умолчанию: 4)\n");
    printf("  -f <файл>      Файл с входными данными (опционально)\n");
    printf("  -h             Вывод этой справки\n");
}

int main(int argc, char* argv[]) {
    int array_size = 1024;
    int num_threads = 4;
    char* filename = NULL;
    
    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            array_size = atoi(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
            num_threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
            filename = argv[++i];
        else if (strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
    }
    
    // Проверка корректности количества потоков
    int available_cores = get_nprocs();
    if (num_threads > available_cores * 2)
        printf("Предупреждение: Запрошено %d потоков, но система имеет только %d ядер\n", num_threads, available_cores);
    
    // Загрузка или генерация массива
    int* array = NULL;
    if (filename) {
        array = read_array_from_file(filename, &array_size);

        if (!array) {
            fprintf(stderr, "Не удалось прочитать массив из файла\n");
            return 1;
        }
    } else {
        // Проверка и корректировка размера до степени двойки
        if (!is_power_of_two(array_size)) {
            int new_size = next_power_of_two(array_size);
            printf("Размер массива изменен с %d на %d (ближайшая степень двойки)\n", 
                   array_size, new_size);
            array_size = new_size;
        }
        
        srand(time(NULL));
        array = generate_random_array(array_size);
        if (!array) {
            fprintf(stderr, "Не удалось сгенерировать массив\n");
            return 1;
        }
    }
    
    printf("=== Параметры выполнения ===\n");
    printf("Размер массива: %d\n", array_size);
    printf("Количество потоков: %d\n", num_threads);
    printf("Количество ядер в системе: %d\n", available_cores);
    printf("Массив загружен из файла: %s\n", filename ? "Да" : "Нет (случайный)");
    
    // Вывод части исходного массива
    printf("\n=== Исходный массив ===\n");
    print_array_partial(array, array_size, 10);
    
    // Копии массива для последовательной и параллельной версий
    int* array_seq = copy_array(array, array_size);
    int* array_par = copy_array(array, array_size);
    
    if (!array_seq || !array_par) {
        fprintf(stderr, "Ошибка копирования массива\n");
        free(array);
        free(array_seq);
        free(array_par);
        return 1;
    }
    
    // Последовательная сортировка
    printf("\n=== Последовательная версия ===\n");
    double start_time = get_time();
    batcher_odd_even_sort_sequential(array_seq, array_size);
    double seq_time = get_time() - start_time;
    
    printf("Время выполнения: %.6f секунд\n", seq_time);
    printf("Массив отсортирован: %s\n", is_sorted(array_seq, array_size) ? "Да" : "Нет");
    print_array_partial(array_seq, array_size, 10);
    
    // Параллельная сортировка
    printf("\n=== Параллельная версия ===\n");
    
    // Инициализация барьера
    if (pthread_barrier_init(&g_barrier, NULL, num_threads) != 0) {
        fprintf(stderr, "Ошибка инициализации барьера\n");
        free(array);
        free(array_seq);
        free(array_par);
        return 1;
    }
    
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = (thread_data_t*)malloc(num_threads * sizeof(thread_data_t));
    
    if (!threads || !thread_data) {
        fprintf(stderr, "Ошибка выделения памяти для потоков\n");
        free(array);
        free(array_seq);
        free(array_par);
        free(threads);
        free(thread_data);
        pthread_barrier_destroy(&g_barrier);
        return 1;
    }
    
    start_time = get_time();
    
    // Создание потоков
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].array = array_par;
        thread_data[i].size = array_size;
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].barrier = &g_barrier;
        
        if (pthread_create(&threads[i], NULL, batcher_odd_even_sort_parallel, &thread_data[i]) != 0) {
            fprintf(stderr, "Ошибка создания потока %d\n", i);
            // Очистка уже созданных потоков
            for (int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
                pthread_join(threads[j], NULL);
            }
            free(array);
            free(array_seq);
            free(array_par);
            free(threads);
            free(thread_data);
            pthread_barrier_destroy(&g_barrier);
            return 1;
        }
    }
    
    // Ожидание завершения потоков
    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);
    
    double par_time = get_time() - start_time;
    
    printf("Время выполнения: %.6f секунд\n", par_time);
    printf("Массив отсортирован: %s\n", is_sorted(array_par, array_size) ? "Да" : "Нет");
    print_array_partial(array_par, array_size, 10);
    
    // Расчет метрик
    printf("\n=== Метрики производительности ===\n");
    double speedup = seq_time / par_time;
    double efficiency = speedup / num_threads;
    
    printf("Ускорение (Speedup): T1/T%d = %.6f/%.6f = %.4f\n", 
           num_threads, seq_time, par_time, speedup);
    printf("Эффективность (Efficiency): S%d/%d = %.4f/%d = %.4f\n", 
           num_threads, num_threads, speedup, num_threads, efficiency);
    
    // Проверка корректности результатов
    printf("Результаты идентичны: %s\n", 
           memcmp(array_seq, array_par, array_size * sizeof(int)) == 0 ? "Да" : "Нет");
    
    // Очистка ресурсов
    pthread_barrier_destroy(&g_barrier);
    free(array);
    free(array_seq);
    free(array_par);
    free(threads);
    free(thread_data);
    
    return 0;
}
