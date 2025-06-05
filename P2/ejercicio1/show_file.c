/**
 * show_file.c
 *
 * Lee el contenido de un fichero y lo vuelca por pantalla usando fread()/fwrite().
 * Versión optimizada respecto al bucle byte a byte (getc/putc).
 *
 * Uso:
 *   ./show_file <ruta_del_fichero>
 *
 * Manuales consultados:
 *   man 3 fopen
 *   man 3 fread
 *   man 3 fwrite
 *   man 3 ferror
 *   man 3 err
 */

#include <stdio.h>    // Declaraciones de FILE, fopen, fread, fwrite, fclose, stderr, stdout
#include <stdlib.h>   // Declaraciones de exit(), EXIT_FAILURE, EXIT_SUCCESS
#include <err.h>      // err(), errx() (extensión GNU para manejar errores con mensaje)

// Tamaño del buffer: número de bytes que leeremos/escribiremos en cada llamada.
// 1024 es un compromiso entre overhead de llamadas y uso de memoria.
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    // Puntero a FILE para la manipulación del fichero.
    FILE *file = NULL;

    // Buffer donde almacenaremos los datos leídos.
    unsigned char buffer[BUFFER_SIZE];

    // bytes_read: número real de bytes devueltos por fread en cada iteración.
    // bytes_written: número de bytes realmente escritos por fwrite.
    size_t bytes_read, bytes_written;

    // 1) Comprobación de argumentos:
    //    argc debe ser 2: [0]=nombre programa, [1]=ruta del fichero
    if (argc != 2) {
        // fprintf escribe en stderr un mensaje de uso y salimos con código 1
        fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 2) Apertura del fichero en modo "rb" (lectura binaria):
    //    - El modo "rb" asegura que fread() lea bytes tal cual,
    //      sin traducción de finales de línea (importante en Windows).
    //    - fopen() devuelve NULL si no puede abrir el fichero.
    file = fopen(argv[1], "rb");
    if (file == NULL) {
        // err(2, ...) formatea e imprime en stderr:
        //   progname: mensaje formato strerror(errno)
        err(2, "No se pudo abrir el fichero de entrada '%s'", argv[1]);
    }

    // 3) Bucle de lectura/escritura:
    //    fread:
    //      - buffer: dirección donde almacenar datos
    //      - 1: tamaño de cada elemento (1 byte)
    //      - BUFFER_SIZE: número máximo de elementos a leer
    //      - file: puntero al FILE abierto
    //    Devuelve el número de elementos leídos (bytes en este caso).
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        // fwrite:
        //   - buffer: datos a escribir
        //   - 1: tamaño de cada elemento (1 byte)
        //   - bytes_read: número de elementos a escribir
        //   - stdout: destino de salida (pantalla)
        bytes_written = fwrite(buffer, 1, bytes_read, stdout);

        // Comprobación de error parcial/total en escritura:
        //   Si fwrite escribe menos bytes de los que pedimos,
        //   algo fue mal (disco lleno, pipe roto, etc.).
        if (bytes_written < bytes_read) {
            fclose(file);
            err(3, "Error al escribir en stdout (fwrite)");
        }
    }

    // 4) Detección de errores de lectura:
    //    fread deja feof(file)=true si llega al final, o
    //    ferror(file)=true si ocurrió un error de lectura.
    if (ferror(file)) {
        // Cierra el fichero antes de salir
        fclose(file);
        err(4, "Error al leer del fichero '%s'", argv[1]);
    }

    // 5) Cierre del fichero:
    //    Siempre debemos liberar el descriptor y el búfer interno.
    if (fclose(file) != 0) {
        // Si fclose falla (caso muy raro), reportamos.
        err(5, "Error al cerrar el fichero '%s'", argv[1]);
    }

    // 6) Salida normal
    return EXIT_SUCCESS;
}
