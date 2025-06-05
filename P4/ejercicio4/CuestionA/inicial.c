/* solución_A.c
 *
 * Cada proceso escribe 5 veces su dígito (0, 1, …, 9) en “output.txt”
 * en la posición fija 5*i, sin carreras, usando pwrite().
 *
 * Compilar:
 *   gcc -Wall -Wextra -std=gnu99 -o solucion_A solución_A.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int main(void) {
    int fd = open("output.txt",
                  O_CREAT | O_TRUNC | O_RDWR,
                  S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    /* Buffer de 5 caracteres + NUL */
    char buf[6];
    buf[5] = '\0';

    for (int i = 0; i < 10; i++) {
        /* Rellenamos buf con cinco veces el carácter '0'+i */
        memset(buf, '0' + i, 5);

        if (i == 0) {
            /* El padre escribe el bloque de ceros en offset 0 */
            if (pwrite(fd, buf, 5, 0) != 5) {
                perror("pwrite padre");
                close(fd);
                return EXIT_FAILURE;
            }
        } else {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                close(fd);
                return EXIT_FAILURE;
            }
            if (pid == 0) {
                /* Hijo: escribe en offset 5*i */
                off_t offset = (off_t)i * 5;
                if (pwrite(fd, buf, 5, offset) != 5) {
                    perror("pwrite hijo");
                    _exit(EXIT_FAILURE);
                }
                _exit(EXIT_SUCCESS);
            }
            /* padre: sigue creando más hijos */
        }
    }

    /* padre espera a todos los hijos */
    while (wait(NULL) > 0) {}

    close(fd);
    return EXIT_SUCCESS;
}
