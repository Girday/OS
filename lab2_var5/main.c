#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <limits.h>


typedef struct {
    int* array;
    int size;
    int thread_id;
    int num_threads;
    int phase;
    pthread_barrier_t* barrier;
} thread_data_t;

int* g_array = NULL;
int g_array_size = 0;
int g_num_threads = 1;
pthread_barrier_t g_barrier;


/* 
    ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
*/

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int is_power_of_two(int n) {
    return (n & (n - 1)) == 0 && n > 0;
}

int next_power_of_two(int n) {
    int power = 1;
    while (power < n)
        power *= 2;
    
    return power;
}


/*
    СОРТИРОВКА БЕТЧЕРА
*/

int batcher_prepare_array(int** array_ptr, int size) {
    if (!is_power_of_two(size)) {
        int new_size = next_power_of_two(size);
        int* new_array = realloc(*array_ptr, new_size * sizeof(int));

        if (!new_array) {
            fprintf(stderr, "Ошибка выделения памяти при увеличении массива\n");
            return size;
        }

        for (int i = size; i < new_size; i++)
            new_array[i] = INT_MAX;

        *array_ptr = new_array;
        printf("Размер массива увеличен с %d до %d (дополнен фиктивными элементами)\n", size, new_size);
        
        size = new_size;
    }

    return size;
}

void batcher_odd_even_sort_sequential(int* array, int size) {
    for (int p = 1; p < size; p *= 2)
        for (int k = p; k >= 1; k /= 2)
            for (int j = k % p; j < size - k; j += 2 * k)
                for (int i = 0; i < k; i++)
                    if ((i + j) / (p * 2) == (i + j + k) / (p * 2))
                        if (array[i + j] > array[i + j + k])
                            swap(&array[i + j], &array[i + j + k]);
}

void* batcher_odd_even_sort_parallel(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int* array = data->array;
    int size = data->size;
    int thread_id = data->thread_id;
    int num_threads = data->num_threads;
    
    for (int p = 1; p < size; p *= 2)
        for (int k = p; k >= 1; k /= 2) {
            for (int j = k % p + thread_id * (2 * k); j < size - k; j += 2 * k * num_threads)
                for (int i = 0; i < k && (i + j) < size - k; i++)
                    if ((i + j) / (p * 2) == (i + j + k) / (p * 2))
                        if (array[i + j] > array[i + j + k])
                            swap(&array[i + j], &array[i + j + k]);
            
            pthread_barrier_wait(data->barrier);
        }

    return NULL;
}


/*
    РАБОТА С МАССИВАМИ
*/

int* generate_random_array(int size) {
    int* array = (int*)malloc(size * sizeof(int));
    if (!array) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return NULL;
    }
    
    for (int i = 0; i < size; i++)
        array[i] = rand();
    
    return array;
}

int* read_array_from_file(const char* filename, int* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла: %s\n", filename);
        return NULL;
    }
    
    int count = 0;
    int temp;
    while (fscanf(file, "%d", &temp) == 1)
        count++;
    
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

int is_sorted(int* array, int size) {
    for (int i = 0; i < size - 1; i++)
        if (array[i] > array[i + 1])
            return 0;

    return 1;
}

int* copy_array(int* src, int size) {
    int* dest = (int*)malloc(size * sizeof(int));
    
    if (!dest) 
        return NULL;
    
    memcpy(dest, src, size * sizeof(int));
    return dest;
}

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


/*
    main и прилегающее
*/

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

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
    int max_threads = 12;
    char* filename = NULL;
    int benchmark_mode = 0;
    int runs = 5;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
            array_size = atoi(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc)
            max_threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
            filename = argv[++i];
        else if (strcmp(argv[i], "--bench") == 0)
            benchmark_mode = 1;
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc)
            runs = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0) {
            print_help();
            printf("  --bench         Режим бенчмарка: строит таблицу для графиков\n");
            return 0;
        }
    }

    srand(time(NULL));

    int* array = NULL;
    if (filename)
        array = read_array_from_file(filename, &array_size);
    else
        array = generate_random_array(array_size);

    if (!array) {
        fprintf(stderr, "Не удалось получить массив\n");
        return 1;
    }

    array_size = batcher_prepare_array(&array, array_size);

//
    if (benchmark_mode) {
        FILE* csv = fopen("results.csv", "w");
        if (!csv) {
            fprintf(stderr, "Ошибка создания файла results.csv\n");
            return 1;
        }

        fprintf(csv, "run,threads,speedup,efficiency,time_seq,time_par\n");

        int* seq_array = copy_array(array, array_size);
        double start = get_time();
        batcher_odd_even_sort_sequential(seq_array, array_size);
        double seq_time = get_time() - start;
        free(seq_array);

        printf("Время последовательной сортировки: %.6f сек\n", seq_time);

        for (int run = 1; run <= runs; run++) {
            printf("\n=== Прогон %d/%d ===\n", run, runs);

            for (int t = 1; t <= max_threads; t++) {
                int* array_par = copy_array(array, array_size);

                pthread_barrier_init(&g_barrier, NULL, t);
                pthread_t* threads = malloc(t * sizeof(pthread_t));
                thread_data_t* thread_data = malloc(t * sizeof(thread_data_t));

                start = get_time();
                for (int i = 0; i < t; i++) {
                    thread_data[i].array = array_par;
                    thread_data[i].size = array_size;
                    thread_data[i].thread_id = i;
                    thread_data[i].num_threads = t;
                    thread_data[i].barrier = &g_barrier;
                    pthread_create(&threads[i], NULL, batcher_odd_even_sort_parallel, &thread_data[i]);
                }

                for (int i = 0; i < t; i++)
                    pthread_join(threads[i], NULL);

                double par_time = get_time() - start;

                pthread_barrier_destroy(&g_barrier);

                free(array_par);
                free(threads);
                free(thread_data);

                double speedup = seq_time / par_time;
                double efficiency = speedup / t;

                fprintf(csv, "%d,%d,%.4f,%.4f,%.6f,%.6f\n",
                        run, t, speedup, efficiency, seq_time, par_time);

                printf("Run %2d | Threads: %2d | Time: %.4fs | Speedup: %.2f | Efficiency: %.2f\n",
                    run, t, par_time, speedup, efficiency);
            }
        }

        fclose(csv);
        printf("\nРезультаты сохранены в файл results.csv\n");
        free(array);
        return 0;
    }
//

    int* array_seq = copy_array(array, array_size);
    int* array_par = copy_array(array, array_size);

    double start_time = get_time();
    batcher_odd_even_sort_sequential(array_seq, array_size);
    double seq_time = get_time() - start_time;

    printf("Последовательная версия: %.6f сек\n", seq_time);

    pthread_barrier_init(&g_barrier, NULL, max_threads);
    pthread_t* threads = malloc(max_threads * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(max_threads * sizeof(thread_data_t));

    start_time = get_time();
    for (int i = 0; i < max_threads; i++) {
        thread_data[i].array = array_par;
        thread_data[i].size = array_size;
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = max_threads;
        thread_data[i].barrier = &g_barrier;
        pthread_create(&threads[i], NULL, batcher_odd_even_sort_parallel, &thread_data[i]);
    }

    for (int i = 0; i < max_threads; i++)
        pthread_join(threads[i], NULL);

    double par_time = get_time() - start_time;

    printf("Параллельная версия: %.6f сек\n", par_time);
    printf("Ускорение: %.3f | Эффективность: %.3f\n", seq_time / par_time, (seq_time / par_time) / max_threads);

    free(array);
    free(array_seq);
    free(array_par);
    
    return 0;
}
