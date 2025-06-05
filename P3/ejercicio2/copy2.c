/*
 * copy2.c
 *
 * Copia de ficheros regulares y enlaces simbólicos usando llamadas POSIX:
 *   - open, read, write, close para ficheros regulares
 *   - lstat, readlink, symlink para enlaces simbólicos
 *
 * Uso:
 *   ./copy2 <origen> <destino>
 *
 * Comportamiento:
 *   1) Usa lstat() para determinar el tipo de "origen":
 *      - Si es un fichero regular, realiza copia bloque a bloque.
 *      - Si es un enlace simbólico, crea un nuevo enlace en "destino"
 *        con la misma ruta que el original.
 *      - Para otros tipos, imprime error y sale.
 *
 * Gestión de errores:
 *   - Comprueba el retorno de todas las llamadas al sistema.
 *   - Imprime mensajes descriptivos usando perror() o strerror(errno).
 *
 * Referencias manual:
 *   man 2 open, read, write, close
 *   man 2 lstat, struct stat
 *   man 2 readlink
 *   man 2 symlink
 *   man 3 strerror
 *
 * Autora: Dorjee
 */

#include <stdio.h>      // perror, fprintf, stderr
#include <stdlib.h>     // exit, EXIT_SUCCESS, EXIT_FAILURE, malloc, free
#include <fcntl.h>      // open flags
#include <unistd.h>     // read, write, close, readlink, symlink
#include <sys/stat.h>   // lstat, struct stat
#include <errno.h>      // errno
#include <string.h>     // strerror

#define BUFFER_SIZE 512  // Tamaño de bloque para ficheros regulares

/**
 * copy_regular:
 *   Copia un fichero regular de "orig" a "dest" bloque a bloque.
 */
void copy_regular(const char *orig, const char *dest) {
    int fd_src, fd_dst;
    ssize_t nread;
    char buffer[BUFFER_SIZE];

    // 1) Abrir origen en solo lectura
    fd_src = open(orig, O_RDONLY);
    if (fd_src == -1) {
        fprintf(stderr, "Error abriendo origen '%s': %s\n", orig, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // 2) Abrir/crear destino en escritura (truncar si existe)
    fd_dst = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dst == -1) {
        fprintf(stderr, "Error abriendo destino '%s': %s\n", dest, strerror(errno));
        close(fd_src);
        exit(EXIT_FAILURE);
    }

    // 3) Bucle de lectura/escritura
    while ((nread = read(fd_src, buffer, BUFFER_SIZE)) > 0) {
        ssize_t total_written = 0;
        // write() puede escribir menos bytes de los pedidos
        while (total_written < nread) {
            ssize_t nw = write(fd_dst,
                               buffer + total_written,
                               nread - total_written);
            if (nw == -1) {
                fprintf(stderr, "Error escribiendo '%s': %s\n", dest, strerror(errno));
                close(fd_src);
                close(fd_dst);
                exit(EXIT_FAILURE);
            }
            total_written += nw;
        }
    }

    // Verificar error de lectura
    if (nread == -1) {
        fprintf(stderr, "Error leyendo '%s': %s\n", orig, strerror(errno));
        close(fd_src);
        close(fd_dst);
        exit(EXIT_FAILURE);
    }

    // 4) Cerrar descriptores
    if (close(fd_src) == -1)
        perror("Error cerrando origen");
    if (close(fd_dst) == -1)
        perror("Error cerrando destino");
}

/**
 * copy_link:
 *   Crea un nuevo enlace simbólico "dest" que apunte
 *   a la misma ruta que el enlace simbólico "orig".
 */
void copy_link(const char *orig, const char *dest) {
    struct stat st;
    char *target_path;
    ssize_t len;

    // 1) Obtener tamaño del enlace (longitud de la ruta sin '\0')
    if (lstat(orig, &st) == -1) {
        fprintf(stderr, "Error en lstat('%s'): %s\n", orig, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // st.st_size es longitud de la ruta de enlace
    target_path = malloc(st.st_size + 1);
    if (!target_path) {
        fprintf(stderr, "malloc() falló\n");
        exit(EXIT_FAILURE);
    }

    // 2) Leer la ruta apuntada por el enlace
    len = readlink(orig, target_path, st.st_size);
    if (len == -1) {
        fprintf(stderr, "Error en readlink('%s'): %s\n", orig, strerror(errno));
        free(target_path);
        exit(EXIT_FAILURE);
    }
    // Asegurar terminador nul
    target_path[len] = '\0';

    // 3) Crear nuevo enlace simbólico
    if (symlink(target_path, dest) == -1) {
        fprintf(stderr, "Error creando symlink '%s' -> '%s': %s\n",
                dest, target_path, strerror(errno));
        free(target_path);
        exit(EXIT_FAILURE);
    }

    free(target_path);
}

int main(int argc, char *argv[]) {
    struct stat st;
    const char *src, *dst;

    // 1) Validar argumentos
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <origen> <destino>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    src = argv[1];
    dst = argv[2];

    // 2) Obtener información del fichero origen sin seguir enlaces
    if (lstat(src, &st) == -1) {
        fprintf(stderr, "Error en lstat('%s'): %s\n", src, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // 3) Actuar según tipo de fichero
    if (S_ISREG(st.st_mode)) {
        // Fichero regular: copia de datos
        copy_regular(src, dst);
    }
    else if (S_ISLNK(st.st_mode)) {
        // Enlace simbólico: replicar enlace
        copy_link(src, dst);
    }
    else {
        // Otros tipos no soportados
        fprintf(stderr, "Tipo de fichero no soportado para '%s'\n", src);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
