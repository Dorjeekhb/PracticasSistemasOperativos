/* solucion_AB.c
 *
 * Ejercicio 4 (Cuestión A y B) unido en un solo programa.
 *
 * Uso:
 *   ./solucion_AB        → modo A (cada dígito escribe en su posición 5·i)
 *   ./solucion_AB -b     → modo B (padre intercala bloques de ceros)
 *
 * Compilar:
 *   gcc -Wall -Wextra -std=gnu99 -o solucion_AB solucion_AB.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

/* Escritura concurrente sin carreras gracias a pwrite() */
static int open_output(void) {
    int fd = open("output.txt",
                  O_CREAT | O_TRUNC | O_RDWR,
                  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}

/* Modo A:
 *  Cada proceso (padre i==0 e hijos i=1..9) escribe 5 dígitos ('0'+i)
 *  en offset = 5*i
 */
static void modo_A(void) {
    int fd = open_output();
    char buf[5];
    for (int i = 0; i < 10; i++) {
        memset(buf, '0' + i, 5);
        if (i == 0) {
            /* bloque 0 en offset 0 */
            if (pwrite(fd, buf, 5, 0) != 5) {
                perror("pwrite A padre");
                close(fd);
                exit(EXIT_FAILURE);
            }
        } else {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                close(fd);
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                /* hijo i escribe en offset 5*i */
                off_t off = (off_t)i * 5;
                if (pwrite(fd, buf, 5, off) != 5) {
                    perror("pwrite A hijo");
                    _exit(EXIT_FAILURE);
                }
                _exit(EXIT_SUCCESS);
            }
            /* padre sigue con los siguientes forks */
        }
    }
    /* padre espera a todos los hijos */
    while (wait(NULL) > 0) {}
    close(fd);
}

/* Modo B:
 *  El padre escribe ceros en offset 0, y luego por cada i=1..9:
 *    - el hijo escribe 'i' en offset (2*i-1)*5
 *    - el padre escribe ceros en offset (2*i)*5
 */
static void modo_B(void) {
    int fd = open_output();
    char buf[5];

    /* bloque de ceros inicial */
    memset(buf, '0', 5);
    if (pwrite(fd, buf, 5, 0) != 5) {
        perror("pwrite B padre inicial");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < 10; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(fd);
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            /* hijo i escribe su dígito en offset (2*i-1)*5 */
            memset(buf, '0' + i, 5);
            off_t off_h = (off_t)(2*i - 1) * 5;
            if (pwrite(fd, buf, 5, off_h) != 5) {
                perror("pwrite B hijo");
                _exit(EXIT_FAILURE);
            }
            _exit(EXIT_SUCCESS);
        } else {
            /* padre intercala ceros en offset (2*i)*5 (solo si i<9) */
            if (i < 9) {
                memset(buf, '0', 5);
                off_t off_p = (off_t)(2*i) * 5;
                if (pwrite(fd, buf, 5, off_p) != 5) {
                    perror("pwrite B padre");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }
            /* y continúa con el siguiente fork */
        }
    }

    /* padre espera a todos los hijos */
    while (wait(NULL) > 0) {}
    close(fd);
}

int main(int argc, char *argv[]) {
    int opt, modoB = 0;
    while ((opt = getopt(argc, argv, "b")) != -1) {
        if (opt == 'b') modoB = 1;
        else {
            fprintf(stderr, "Usage: %s [-b]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (modoB) {
        modo_B();
        printf("Modo B completado: intercalado de ceros y dígitos.\n");
    } else {
        modo_A();
        printf("Modo A completado: bloques de dígitos en posición fija.\n");
    }

    return EXIT_SUCCESS;
}
