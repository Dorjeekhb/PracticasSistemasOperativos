/*
 * cocinero.c
 *
 * Programa “cocinero” para el problema de los salvajes y el caldero.
 * Crea memoria compartida y semáforos, gestiona señales SIGINT/SIGTERM
 * para limpieza, y ejecuta la rutina cook() que repone el caldero.
 *
 * Compilación:
 *   gcc -Wall -Wextra -std=gnu99 -pthread -o cocinero cocinero.c -lrt
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>       // printf, perror
#include <stdlib.h>      // exit, EXIT_FAILURE
#include <unistd.h>      // ftruncate, getpid
#include <signal.h>      // sigaction, SIGINT, SIGTERM
#include <fcntl.h>       // O_CREAT, O_RDWR
#include <sys/mman.h>    // shm_open, mmap, munmap
#include <sys/stat.h>    // mode constants
#include <semaphore.h>   // sem_open, sem_wait, sem_post, sem_close, sem_unlink

// Nombre de recursos POSIX (comparten namespace del sistema)
#define SHM_NAME    "/pot_shm"
#define SEM_MUTEX   "/mutex_sem"
#define SEM_FULL    "/full_sem"
#define SEM_EMPTY   "/empty_sem"

// Tamaño inicial del caldero (número de raciones)
#define M 10

// Flag para indicar al cocinero que termine
static volatile sig_atomic_t finish = 0;

// Descriptores de recurso globales
static sem_t  *mutex_sem  = NULL;
static sem_t  *full_sem   = NULL;
static sem_t  *empty_sem  = NULL;
static int    *servings   = NULL;  // apuntador a memoria compartida
static int     shm_fd     = -1;

// Handler de SIGINT/SIGTERM: despierta al cocinero para que salga
void handler(int signo) {
    finish = 1;
    // Aseguramos que, si el cocinero está bloqueado en empty_sem,
    // reciba un post para poder comprobar finish y salir.
    sem_post(empty_sem);
}

// Repón M raciones en el caldero.  
// Se llama siempre con finish==0 inicialmente, y sem_wait(empty_sem) garantiza
// que se ejecute solo cuando el caldero esté vacío.
void putServingsInPot(int count) {
    // Espero a que un salvaje me avise de “caldero vacío”
    sem_wait(empty_sem);
    if (finish) return;  // si ya toca terminar, salgo

    // Sección crítica para actualizar el contador de raciones
    sem_wait(mutex_sem);
    *servings = count;
    printf("Cook %d: refill pot to %d servings\n", getpid(), count);
    sem_post(mutex_sem);

    // Despierto a todos los salvajes que esperan por raciones
    for (int i = 0; i < count; i++) {
        sem_post(full_sem);
    }
}

// Bucle principal del cocinero: repone cada vez que recibe señal de “vacío”
void cook(void) {
    while (!finish) {
        putServingsInPot(M);
    }
    printf("Cook %d: terminating\n", getpid());
}

int main(void) {
    // 1) Instalación de handler para SIGINT y SIGTERM
    struct sigaction sa = { .sa_handler = handler };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // 2) Crear memoria compartida para el contador de raciones
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) { perror("shm_open"); exit(EXIT_FAILURE); }
    ftruncate(shm_fd, sizeof(int));
    servings = mmap(NULL, sizeof(int),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED, shm_fd, 0);
    if (servings == MAP_FAILED) { perror("mmap"); exit(EXIT_FAILURE); }
    *servings = 0;  // empezamos con caldero vacío

    // 3) Crear y/o inicializar semáforos nombrados
    //    - mutex_sem: exclusión mutua para acceso a *servings
    //    - full_sem: conteo de raciones disponibles (0 al inicio)
    //    - empty_sem: señal de “vacío” (1 al inicio para que el cocinero ponga la primera tanda)
    mutex_sem = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    full_sem  = sem_open(SEM_FULL,  O_CREAT, 0666, 0);
    empty_sem = sem_open(SEM_EMPTY, O_CREAT, 0666, 1);
    if (mutex_sem == SEM_FAILED || full_sem == SEM_FAILED || empty_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // 4) Ejecutar bucle del cocinero hasta señal de término
    cook();

    // 5) Limpieza de recursos
    sem_close(mutex_sem);
    sem_close(full_sem);
    sem_close(empty_sem);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_EMPTY);

    munmap(servings, sizeof(int));
    shm_unlink(SHM_NAME);
    close(shm_fd);

    return EXIT_SUCCESS;
}
