#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SHM_PATH "/tmp/lab3_mmap"
#define STATUS_IDLE   0
#define STATUS_ACK    1
#define STATUS_FINISH 2

struct shm_region {
    sem_t sem_parent;
    sem_t sem_child;
    int number;
    int status;
};

static void perror_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int open_and_map_shm(struct shm_region **out) {
    int fd = open(SHM_PATH, O_RDWR);
    if (fd == -1) return -1;
    size_t sz = sizeof(struct shm_region);
    void *addr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return -1;
    }
    *out = (struct shm_region *)addr;
    close(fd);
    return 0;
}

int isPrime(int n) {
    if (n <= 1)
        return 0;

    for (int i = 2; i * i <= n; i++)
        if (n % i == 0)
            return 0;

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *outfile = argv[1];

    struct shm_region *shm;
    if (open_and_map_shm(&shm) == -1) {
        perror_exit("open_and_map_shm");
    }

    int fd = open(outfile, O_WRONLY | O_CREAT, 0644);
    if (fd == -1)
        perror("open output file");

    while (1) {
        if (sem_wait(&shm->sem_parent) == -1) {
            perror("sem_wait sem_parent");
            break;
        }

        int num = shm->number;

        if (num < 0) {
            shm->status = STATUS_FINISH;
            if (sem_post(&shm->sem_child) == -1) 
                perror("sem_post sem_child");
            break;
        }

        if (isPrime(num)) {
            shm->status = STATUS_FINISH;
            if (sem_post(&shm->sem_child) == -1) 
                perror("sem_post sem_child");
            break;
        } else {
            if (fd != -1) {
                if (lseek(fd, 0, SEEK_END) == -1)
                    perror("lseek");
                
                char buf[64];
                int len = snprintf(buf, sizeof(buf), "%d\n", num);
                
                if (write(fd, buf, (size_t)len) != len)
                    perror("write to output file");
            }

            shm->status = STATUS_ACK;
            if (sem_post(&shm->sem_child) == -1) 
                perror("sem_post sem_child");
        }
    }

    if (fd != -1) 
        close(fd);
    if (munmap(shm, sizeof(struct shm_region)) == -1) 
        perror("munmap");

    return 0;
}
