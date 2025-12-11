#define _XOPEN_SOURCE 700 // POSIX.1-2008

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SHM_PATH "/tmp/lab3_mmap"
#define SHM_MODE 0666

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

int create_and_map_shm(struct shm_region **out) {
    int fd = open(SHM_PATH, O_RDWR | O_CREAT, SHM_MODE);
    if (fd == -1) 
        return -1;

    size_t sz = sizeof(struct shm_region);
    if (ftruncate(fd, sz) == -1) {
        close(fd);
        return -1;
    }

    void *addr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        close(fd);
        return -1;
    }

    *out = (struct shm_region *)addr;
    close(fd);

    return 0;
}

int main() {
    struct shm_region *shm;
    if (create_and_map_shm(&shm) == -1)
        perror_exit("create_and_map_shm");

    shm->number = 0;
    shm->status = STATUS_IDLE;
    
    if (sem_init(&shm->sem_parent, 1, 0) == -1) 
        perror_exit("sem_init sem_parent");
    
    if (sem_init(&shm->sem_child, 1, 0) == -1) 
        perror_exit("sem_init sem_child");

    char out_fname[256];
    printf("Please, enter the NAME of the FILE: ");
    fflush(stdout);

    if (scanf("%255s", out_fname) != 1) {
        fprintf(stderr, "Failed to read filename\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) 
        perror_exit("fork");

    if (pid == 0) {
        execl("./child.out", "child.out", out_fname, NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    printf("Enter numbers (one per line):\n");
    fflush(stdout);

    while (1) {
        int num;
        int scan_ok = (scanf("%d", &num) == 1);

        if (!scan_ok) {
            num = -1;
            printf("EOF/input error — sending termination (-1) to child.\n");
        }

        shm->number = num;
        shm->status = STATUS_IDLE;
        if (sem_post(&shm->sem_parent) == -1) {
            perror("sem_post sem_parent");
            break;
        }

        if (sem_wait(&shm->sem_child) == -1) {
            perror("sem_wait sem_child");
            break;
        }

        if (shm->status == STATUS_FINISH)
            break;
        else if (shm->status == STATUS_ACK) {
            if (!scan_ok)
                break;
            continue;
        } else {
            fprintf(stderr, "Unexpected status from child: %d\n", shm->status);
            break;
        }

        if (!scan_ok) 
            break;
    }

    wait(NULL);

    if (sem_destroy(&shm->sem_parent) == -1) 
        perror("sem_destroy sem_parent");
    
    if (sem_destroy(&shm->sem_child) == -1) 
        perror("sem_destroy sem_child");

    if (munmap(shm, sizeof(struct shm_region)) == -1) 
        perror("munmap");

    if (unlink(SHM_PATH) == -1)
        fprintf(stderr, "Warning: unlink(%s) failed: %s\n", SHM_PATH, strerror(errno));

    return 0;
}
