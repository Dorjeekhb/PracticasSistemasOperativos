/*
 * copy.c
 *
 * Copia un fichero regular bloque a bloque (512 B) usando llamadas
 * al sistema POSIX: open, read, write y close.
 *
 * Uso:
 *   ./copy <fichero_origen> <fichero_destino>
 *
 * Descripción:
 *   - Abre el fichero origen en modo solo lectura (O_RDONLY).
 *   - Abre o crea el fichero destino en modo escritura (O_WRONLY | O_CREAT | O_TRUNC),
 *     con permisos rw-r--r-- (0644). Si el destino existe, se vacía su contenido.
 *   - Lee bloques de 512 bytes del origen y escribe exactamente esos bytes en el destino.
 *     El tamaño de lectura puede ser menor al final del fichero.
 *   - Gestiona escrituras parciales: write() puede escribir menos bytes de los solicitados,
 *     por lo que emplea un bucle hasta completar la escritura del bloque leído.
 *   - Verifica errores en cada llamada al sistema y muestra mensajes descriptivos.
 *
 * Funciones principales:
 *   - open(2): abrir/crear ficheros con flags O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC.
 *   - read(2): leer datos en buffer, devuelve número de bytes o -1 si error.
 *   - write(2): escribir datos desde buffer, devuelve bytes escritos o -1 si error.
 *   - close(2): cerrar descriptor.
 *   - strerror(errno): obtener descripción de error.
 *
 * Tamaño de bloque: 512 bytes.
 *
 * Referencias manual:
 *   man 2 open
 *   man 2 read
 *   man 2 write
 *   man 2 close
 *   man 3 strerror
 *
 * Autora: Dorjee
 */

#include <stdio.h>      // fprintf, perror, stderr
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include <fcntl.h>      // open flags: O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>     // read, write, close
#include <string.h>     // strerror
#include <errno.h>      // errno

#define BUFFER_SIZE 512  // Tamaño fijo de cada bloque de E/S

/**
 * copy: copia datos desde fdo a fdd en bloques de BUFFER_SIZE.
 * Gestiona lecturas parciales al final y escrituras parciales.
 * Sale con EXIT_FAILURE en caso de error.
 */
void copy(int fdo, int fdd) {
    ssize_t nread;
    char buffer[BUFFER_SIZE];

    // Leer hasta EOF: read() devuelve 0
    while ((nread = read(fdo, buffer, BUFFER_SIZE)) > 0) {
        ssize_t total_written = 0;
        // Escribir todos los bytes leídos
        while (total_written < nread) {
            ssize_t nw = write(fdd,
                               buffer + total_written,
                               nread - total_written);
            if (nw == -1) {
                fprintf(stderr, "Error escribiendo destino: %s\n",
                        strerror(errno));
                exit(EXIT_FAILURE);
            }
            total_written += nw;
        }
    }

    // Comprobar error de lectura
    if (nread == -1) {
        fprintf(stderr, "Error leyendo origen: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Si nread == 0: EOF alcanzado correctamente
}

int main(int argc, char *argv[]) {
    // Validar argumentos: se requieren 2 parámetros
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <fichero_origen> <fichero_destino>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *src = argv[1];
    const char *dst = argv[2];

    // Abrir fichero origen en solo lectura
    int fd_src = open(src, O_RDONLY);
    if (fd_src == -1) {
        fprintf(stderr, "Error abriendo origen '%s': %s\n",
                src, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Abrir/crear fichero destino en modo escritura y truncado
    int fd_dst = open(dst,
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644 /* rw-r--r-- */);
    if (fd_dst == -1) {
        fprintf(stderr, "Error abriendo destino '%s': %s\n",
                dst, strerror(errno));
        close(fd_src);
        exit(EXIT_FAILURE);
    }

    // Realizar la copia de datos
    copy(fd_src, fd_dst);

    // Cerrar descriptores
    if (close(fd_src) == -1) {
        perror("Error cerrando origen");
    }
    if (close(fd_dst) == -1) {
        perror("Error cerrando destino");
    }

    return EXIT_SUCCESS;
}
