/* solución_B.c
 *
 * Igual que A, pero el padre intercala su bloque “00000”:
 * 0 → 1 → 0 → 2 → 0 → 3 … → 0 → 9
 *
 * Compilar:
 *   gcc -Wall -Wextra -std=gnu99 -o solucion_B solución_B.c
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

    char buf[6];
    buf[5] = '\0';

    /* Paso 0: padre escribe ceros en offset 0 */
    memset(buf, '0', 5);
    if (pwrite(fd, buf, 5, 0) != 5) {
        perror("pwrite padre 0");
        close(fd);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < 10; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(fd);
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            /* HIJO: escribe cinco dígitos '0'+i en offset (2*i−1)*5 */
            memset(buf, '0' + i, 5);
            off_t child_off = (off_t)(2*i - 1) * 5;
            if (pwrite(fd, buf, 5, child_off) != 5) {
                perror("pwrite hijo");
                _exit(EXIT_FAILURE);
            }
            _exit(EXIT_SUCCESS);
        } else {
            /* PADRE: escribe ceros en offset (2*i)*5, salvo tras i=9 */
            off_t parent_off = (off_t)(2*i) * 5;
            if (i < 9) {
                memset(buf, '0', 5);
                if (pwrite(fd, buf, 5, parent_off) != 5) {
                    perror("pwrite padre");
                    close(fd);
                    return EXIT_FAILURE;
                }
            }
            /* y sigue creando más hijos */
        }
    }

    /* padre espera a todos los hijos */
    while (wait(NULL) > 0) {}

    close(fd);
    return EXIT_SUCCESS;
}
