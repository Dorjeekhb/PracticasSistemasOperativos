/**
 * read_strings.c
 *
 * Lee de un fichero cadenas terminadas en '\0' y las muestra por pantalla,
 * una por línea. Cada llamada a loadstr() devuelve la siguiente cadena
 * almacenada (incluyendo su terminador), asignando memoria dinámica.
 *
 * Uso:
 *   ./read_strings <ruta_fichero>
 *
 * Manuales consultados:
 *   man 3 fopen
 *   man 3 fread
 *   man 3 fseek
 *   man 3 ftell
 *   man 3 ungetc
 *   man 3 malloc
 *   man 3 err
 */

#include <stdio.h>    // fopen, fread, fgetc, ungetc, fseek, ftell, fclose, stderr, stdout
#include <stdlib.h>   // malloc, free, exit, EXIT_FAILURE, EXIT_SUCCESS
#include <err.h>      // err()
#include <string.h>   // (solo para comentarios; no usamos strlen aquí)

/**
 * loadstr:
 *   Lee una cadena terminada en '\0' desde la posición actual de 'file'.
 *   - Determina cuántos bytes tiene (contando el '\0') leyendo byte a byte.
 *   - Retrocede el puntero al inicio de la cadena.
 *   - Reserva con malloc() la memoria necesaria y lee de nuevo la cadena completa.
 *
 * Parámetro:
 *   file: descriptor de fichero abierto en modo lectura binaria.
 *
 * Retorno:
 *   Apuntador a la cadena (malloc), terminado en '\0'.
 *   NULL si estamos ya al final de fichero (EOF).
 *   En caso de error irrecuperable, termina el programa con err().
 */
char *loadstr(FILE *file)
{
    long start_pos;
    int ch;
    size_t length = 0;
    char *buffer;

    /* 1) Guardar posición inicial (ftell) */
    start_pos = ftell(file);
    if (start_pos == -1L) {
        err(5, "ftell() falló");
    }

    /* 2) Miramos el primer byte para detectar EOF inmediato */
    ch = fgetc(file);
    if (ch == EOF) {
        if (feof(file))
            return NULL;        // sin más cadenas
        else
            err(6, "Error al leer primer byte");
    }
    /* Devolver ese byte al flujo para no perder datos */
    if (ungetc(ch, file) == EOF) {
        err(7, "ungetc() falló");
    }

    /* 3) Contar cuántos bytes hay hasta (e incluyendo) el '\0' */
    while ((ch = fgetc(file)) != EOF) {
        length++;
        if (ch == '\0')
            break;               // hemos leído el terminador
    }
    if (ch == EOF) {
        err(8, "EOF inesperado antes del terminador '\\0'");
    }

    /* 4) Volver al inicio de la cadena */
    if (fseek(file, start_pos, SEEK_SET) != 0) {
        err(9, "fseek() falló");
    }

    /* 5) Reservar memoria exacta (length bytes, incluye '\0') */
    buffer = malloc(length);
    if (buffer == NULL) {
        err(10, "malloc() falló al reservar %zu bytes", length);
    }

    /* 6) Leer la cadena completa de una sola vez */
    if (fread(buffer, 1, length, file) < length) {
        err(11, "fread() no pudo leer %zu bytes", length);
    }

    return buffer;
}

int main(int argc, char *argv[])
{
    FILE *in;
    char *str;

    /* 1) Comprobación de argumentos */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 2) Apertura del fichero en modo binario-lectura */
    in = fopen(argv[1], "rb");
    if (in == NULL) {
        err(2, "No se pudo abrir el fichero '%s'", argv[1]);
    }

    /* 3) Leer y mostrar todas las cadenas */
    while ((str = loadstr(in)) != NULL) {
        /* printf detiene al encontrar '\0' */
        printf("%s\n", str);
        free(str);    // liberar la memoria asignada por loadstr
    }

    /* 4) Cierre del fichero */
    if (fclose(in) != 0) {
        err(12, "Error al cerrar el fichero '%s'", argv[1]);
    }

    return EXIT_SUCCESS;
}
