/**
 * write_strings.c
 *
 * Escribe en un fichero un conjunto de cadenas terminadas en '\0'.
 * Cada string recibido en argv se almacena con su terminador en el fichero.
 *
 * Uso:
 *   ./write_strings <ruta_fichero> <string1> [string2 ...]
 *
 * Manuales consultados:
 *   man 3 fopen
 *   man 3 fwrite
 *   man 3 strlen
 *   man 3 err
 */

#include <stdio.h>    // fopen, fwrite, fclose, stderr, stdout
#include <stdlib.h>   // exit, EXIT_FAILURE, EXIT_SUCCESS, malloc
#include <string.h>   // strlen
#include <err.h>      // err()

int main(int argc, char* argv[])
{
    FILE *out = NULL;
    size_t i, len, written;

    /* 1) Comprobación de argumentos:
     *    argc >= 2: argv[1] = nombre de fichero de salida; argv[2..] = cadenas a escribir
     */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_name> [string1] [string2] ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 2) Apertura del fichero en modo binario-escritura ("wb"):
     *    - Si existe, se trunca (se elimina su contenido anterior).
     *    - Si no existe, se crea.
     */
    out = fopen(argv[1], "wb");
    if (out == NULL) {
        err(2, "No se pudo abrir el fichero de salida '%s'", argv[1]);
    }

    /* 3) Para cada cadena en argv[2..argc-1]:
     *    - Calculamos longitud +1 para incluir el '\0' final.
     *    - fwrite() escribe en bloque [cadena + terminador].
     */
    for (i = 2; i < (size_t)argc; i++) {
        len = strlen(argv[i]) + 1;           // +1 para el '\0'
        written = fwrite(argv[i], 1, len, out);
        if (written < len) {
            /* Si no escribe todos los bytes, hay un fallo (disco lleno, pipe roto...) */
            fclose(out);
            err(3, "Error al escribir la cadena '%s' en '%s'", argv[i], argv[1]);
        }
    }

    /* 4) Cierre del fichero de salida */
    if (fclose(out) != 0) {
        err(4, "Error al cerrar el fichero '%s'", argv[1]);
    }

    return EXIT_SUCCESS;
}
