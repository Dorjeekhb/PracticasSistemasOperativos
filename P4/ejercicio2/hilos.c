/* hilos.c
 *
 * Ejercicio 2: Creación y paso de parámetros a hilos usando pthreads.
 *
 * Cada hilo recibe un puntero a una estructura dinámica con dos campos:
 *  - num   : número de hilo
 *  - prio  : 'P' si es prioritario (num par), 'N' si no (num impar)
 *
 * El hilo:
 * 1) copia los datos en variables locales
 * 2) libera la memoria dinámica de arg
 * 3) obtiene su propio ID con pthread_self()
 * 4) imprime: [thread 0x... num=5 prio=N]
 *
 * Al final, el main hace pthread_join() de todos ellos.
 *
 * Uso:
 *   ./hilos <número_de_hilos>
 *
 * Compilar:
 *   gcc -Wall -Wextra -std=gnu99 -pthread -o hilos hilos.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     // sleep()
#include <errno.h>
#include <string.h>     // strerror()

// Estructura que pasaremos a cada hilo:
//   num  = número de hilo
//   prio = 'P' o 'N'
typedef struct {
    int   num;
    char  prio;
} hilo_arg_t;

void *thread_usuario(void *arg) {
    // 1) Convertir void* a nuestro tipo
    hilo_arg_t *datos = (hilo_arg_t *)arg;

    // 2) Copiar en variables locales
    int   mi_num  = datos->num;
    char  mi_prio = datos->prio;

    // 3) Liberar la memoria dinámica ya que no la volveremos a usar
    free(datos);

    // 4) Obtener el ID del hilo
    pthread_t tid = pthread_self();

    // 5) Imprimir mensaje
    printf("[thread %lu num=%d prio=%c]\n",
           (unsigned long)tid, mi_num, mi_prio);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <num_hilos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Número de hilos debe ser > 0\n");
        return EXIT_FAILURE;
    }

    // Array para guardar los IDs de hilo
    pthread_t *tids = malloc(n * sizeof(pthread_t));
    if (!tids) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    // 1) Creación de hilos
    for (int i = 0; i < n; i++) {
        // Reservamos un bloque para pasar como argumento
        hilo_arg_t *arg = malloc(sizeof(hilo_arg_t));
        if (!arg) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        arg->num  = i;
        arg->prio = (i % 2 == 0 ? 'P' : 'N');

        // Lanzamos el hilo, pasando arg; guardamos el pthread_t
        int err = pthread_create(&tids[i], NULL, thread_usuario, arg);
        if (err) {
            fprintf(stderr, "pthread_create[%d]: %s\n", i, strerror(err));
            free(arg);
            // continuar intentando crear los demás
        }
    }

    // 2) Esperar a que todos los hilos terminen
    for (int i = 0; i < n; i++) {
        pthread_join(tids[i], NULL);
    }

    free(tids);
    return EXIT_SUCCESS;
}
