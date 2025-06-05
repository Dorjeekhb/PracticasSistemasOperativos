/*
 * apertura.c
 *
 * Prueba la apertura de un fichero en distintos modos (lectura, escritura,
 * lectura/escritura), crea o trunca el fichero según corresponda, e intenta
 * realizar una operación de escritura y otra de lectura, informando de errores.
 *
 * Uso:
 *   ./apertura.x -f <fichero> [-r] [-w]
 *
 *   -f <fichero>   Fichero a abrir (obligatorio).
 *   -r             Abrir en modo lectura (O_RDONLY).
 *   -w             Abrir en modo escritura (O_WRONLY).
 *                  Si no existe, se creará; si existe y -w, se trunca.
 *   -r y -w juntos abren en modo lectura/escritura (O_RDWR).
 *
 * Ejemplos:
 *   chmod 600 archivo.txt
 *   ./apertura.x -f archivo.txt -r      # lectura: read OK, write -> EBADF
 *   ./apertura.x -f archivo.txt -w      # escritura: write OK, read -> EBADF
 *   ./apertura.x -f archivo.txt -rw     # lectura/escritura: ambas OK
 *
 *   chmod a-r archivo.txt
 *   ./apertura.x -f archivo.txt -r      # open -> EACCES
 *
 *   chmod -x apertura.x
 *   ./apertura.x ...                    # ejecución -> Permission denied
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // getopt
#include <fcntl.h>      // open flags
#include <errno.h>
#include <string.h>
#include <sys/stat.h>   // file modes
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/unistd.h> // read, write, close

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s -f <file> [-r] [-w]\n"
        "  -f <file>   fichero a abrir (obligatorio)\n"
        "  -r          modo lectura (O_RDONLY)\n"
        "  -w          modo escritura (O_WRONLY)\n"
        "             (-r y -w juntos = O_RDWR)\n",
        prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int opt;
    const char *filename = NULL;
    int opt_r = 0, opt_w = 0;

    // Parseo de opciones
    while ((opt = getopt(argc, argv, "f:rw")) != -1) {
        switch (opt) {
            case 'f':
                filename = optarg;
                break;
            case 'r':
                opt_r = 1;
                break;
            case 'w':
                opt_w = 1;
                break;
            default:
                usage(argv[0]);
        }
    }

    if (!filename || (!opt_r && !opt_w)) {
        fprintf(stderr, "Error: -f y al menos -r o -w son obligatorios\n");
        usage(argv[0]);
    }

    // Construcción de flags de open()
    int flags = 0;
    if (opt_r && opt_w) {
        flags = O_RDWR;
    } else if (opt_r) {
        flags = O_RDONLY;
    } else { // solo escritura
        flags = O_WRONLY;
    }
    flags |= O_CREAT;
    if (opt_w) {
        flags |= O_TRUNC;
    }

    // Permisos en caso de creación: rw-rw-rw- (022 umask se aplica)
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

    // Intentar abrir (y posiblemente crear/truncar) el fichero
    int fd = open(filename, flags, mode);
    if (fd < 0) {
        fprintf(stderr, "Error en open('%s'): %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("open('%s', flags=0x%x) = %d\n", filename, flags, fd);

    // 1) Intento de escritura
    const char *msg = "X";
    ssize_t nw = write(fd, msg, 1);
    if (nw < 0) {
        fprintf(stderr, "write() -> Error: %s\n", strerror(errno));
    } else {
        printf("write() escribió %zd bytes\n", nw);
    }

    // 2) Intento de lectura
    // No retrocedemos el offset: si abrimos O_RDWR, read puede devolver 0 (EOF).
    char buf[16];
    ssize_t nr = read(fd, buf, sizeof(buf));
    if (nr < 0) {
        fprintf(stderr, "read()  -> Error: %s\n", strerror(errno));
    } else {
        printf("read() devolvió %zd bytes\n", nr);
    }

    // Cerrar descriptor
    if (close(fd) < 0) {
        fprintf(stderr, "close() -> Error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
