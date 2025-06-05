/*
 * salvajes.c
 *
 * Programa “salvaje” que abre los semáforos y memoria creada por cocinero,
 * ejecuta NUMITER veces getServingsFromPot()→eat() y termina.
 *
 * Compilación:
 *   gcc -Wall -Wextra -std=gnu99 -pthread -o salvajes salvajes.c -lrt
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit, EXIT_FAILURE, rand, srand
#include <unistd.h>     // sleep, getpid
#include <fcntl.h>      // O_RDWR
#include <sys/mman.h>   // shm_open, mmap, munmap
#include <sys/stat.h>   // constants
#include <semaphore.h>  // sem_open, sem_wait, sem_post, sem_close

#define SHM_NAME    "/pot_shm"
#define SEM_MUTEX   "/mutex_sem"
#define SEM_FULL    "/full_sem"
#define SEM_EMPTY   "/empty_sem"

// Número de veces que cada salvaje come
#define NUMITER  3

// Cantidad de raciones que consume en cada iteración (por defecto 1, se puede
// modificar mediante argumento de línea de órdenes)
static int portion_size = 1;

static sem_t *mutex_sem = NULL;
static sem_t *full_sem  = NULL;
static sem_t *empty_sem = NULL;
static int   *servings  = NULL;
static int    shm_fd    = -1;

/**
 * getServingsFromPot:
 *   - Espera a que haya una ración disponible (sem_wait(full_sem)).
 *   - En sección crítica decrementa *servings.
 *   - Si queda 0, despierta al cocinero (sem_post(empty_sem)).
 */
int getServingsFromPot(void) {
    pid_t id = getpid();

    // 1) Esperar ración disponible
    sem_wait(full_sem);

    // 2) Entrar sección crítica
    sem_wait(mutex_sem);
    (*servings)--;
    printf("Savage %d: took serving, remaining=%d\n", id, *servings);

    // 3) Si justo tomé la última, aviso al cocinero
    if (*servings == 0) {
        printf("Savage %d: pot empty, waking cook\n", id);
        sem_post(empty_sem);
    }
    sem_post(mutex_sem);

    return 1;
}

// Simula comer: imprime mensaje y duerme un tiempo aleatorio
void eat(int amount) {
    pid_t id = getpid();
    printf("Savage %d: eating %d serving(s)\n", id, amount);
    sleep(rand() % 5 + 1);
}

// Rutina principal: comer NUMITER veces
void savages(void) {
    srand(getpid());  // semilla distinta por proceso
    for (int i = 0; i < NUMITER; i++) {
        for (int j = 0; j < portion_size; j++) {
            getServingsFromPot();
        }
        eat(portion_size);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        portion_size = atoi(argv[1]);
        if (portion_size <= 0) {
            fprintf(stderr, "Invalid portion size: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }
    // 1) Abrir memoria compartida existente
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd < 0) {
        perror("shm_open");
        fprintf(stderr, "Run 'cocinero' first to create resources.\n");
        exit(EXIT_FAILURE);
    }
    servings = mmap(NULL, sizeof(int),
                    PROT_READ|PROT_WRITE,
                    MAP_SHARED, shm_fd, 0);
    if (servings == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // 2) Abrir semáforos existentes
    mutex_sem = sem_open(SEM_MUTEX, 0);
    full_sem  = sem_open(SEM_FULL,  0);
    empty_sem = sem_open(SEM_EMPTY, 0);
    if (mutex_sem == SEM_FAILED || full_sem == SEM_FAILED || empty_sem == SEM_FAILED) {
        perror("sem_open");
        fprintf(stderr, "Run 'cocinero' first to create semaphores.\n");
        exit(EXIT_FAILURE);
    }

    // 3) Ejecutar la rutina de salvaje
    savages();

    // 4) Limpieza local (no eliminamos recursos globales)
    sem_close(mutex_sem);
    sem_close(full_sem);
    sem_close(empty_sem);

    munmap(servings, sizeof(int));
    close(shm_fd);
    return EXIT_SUCCESS;
}
