/*
 * mostrar.c
 *
 * Programa similar a 'cat' que muestra el contenido de un fichero regular,
 * con opción de saltarse los primeros N bytes o de mostrar solo los últimos N bytes.
 *
 * Uso:
 *   ./mostrar [-n N] [-e] <ruta_fichero>
 *
 * Opciones:
 *   -n N   Número de bytes a saltar (por defecto 0) o a mostrar si -e está presente
 *   -e     Indica que se deben mostrar los últimos N bytes en lugar de saltar los primeros
 *
 * Comportamiento:
 *   1) Abrir el fichero en lectura.
 *   2) Parsear con getopt() las opciones -n y -e.
 *   3) Posicionar el descriptor de fichero con lseek():
 *        - Si -e: lseek(fd, -N, SEEK_END) para avanzar hasta N desde el final.
 *        - Si no -e: lseek(fd, N, SEEK_SET) para saltar N desde el comienzo.
 *   4) Leer byte a byte (read(fd, &c, 1)) y escribir en stdout (write(1, &c, 1)) hasta EOF.
 *
 * Páginas de manual:
 *   man 2 open, man 2 read, man 2 write, man 2 close, man 2 lseek
 *   man 3 getopt, man 3 perror, man 3 strerror
 *
 * Autora: Dorjee
 */

#include <stdio.h>      // perror, fprintf, stderr
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE, strtol
#include <fcntl.h>      // open, O_RDONLY
#include <unistd.h>     // read, write, close, lseek, getopt
#include <errno.h>      // errno
#include <string.h>     // strerror

int main(int argc, char *argv[]) {
    int opt;
    long N = 0;
    int show_last = 0;
    char *endptr;

    // 1) Procesar opciones -n y -e
    while ((opt = getopt(argc, argv, "n:e")) != -1) {
        switch (opt) {
        case 'n':
            // Convertir argumento N a número
            N = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || N < 0) {
                fprintf(stderr, "Opción -n requiere un número no negativo\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'e':
            show_last = 1;
            break;
        default:
            fprintf(stderr, "Uso: %s [-n N] [-e] <fichero>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // 2) Comprobar argumento fichero
    if (optind >= argc) {
        fprintf(stderr, "Falta especificar el fichero\n");
        fprintf(stderr, "Uso: %s [-n N] [-e] <fichero>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *path = argv[optind];

    // 3) Abrir fichero en modo solo lectura
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error abriendo '%s': %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // 4) Posicionar marcador con lseek
    off_t offset;
    if (show_last) {
        // Mostrar últimos N bytes: mover N desde final (pos puede ser negativo)
        offset = lseek(fd, -N, SEEK_END);
        if (offset == (off_t)-1) {
            fprintf(stderr, "Error en lseek SEEK_END: %s\n", strerror(errno));
            close(fd);
            exit(EXIT_FAILURE);
        }
    } else {
        // Saltar primeros N bytes: mover N desde inicio
        offset = lseek(fd, N, SEEK_SET);
        if (offset == (off_t)-1) {
            fprintf(stderr, "Error en lseek SEEK_SET: %s\n", strerror(errno));
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    // 5) Leer byte a byte y escribir en stdout
    unsigned char c;
    ssize_t n;
    while ((n = read(fd, &c, 1)) > 0) {
        if (write(STDOUT_FILENO, &c, 1) != 1) {
            perror("Error escribiendo en stdout");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    if (n == -1) {
        perror("Error leyendo fichero");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 6) Cerrar fichero
    if (close(fd) == -1) {
        perror("Error cerrando fichero");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
