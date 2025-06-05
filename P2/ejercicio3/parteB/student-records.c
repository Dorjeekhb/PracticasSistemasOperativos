#include <stdio.h>     // fopen, fgets, fprintf, printf, stderr
#include <stdlib.h>    // EXIT_SUCCESS, EXIT_FAILURE, atoi
#include <unistd.h>    // getopt
#include <string.h>    // strchr, strsep, strncpy, strlen
#include <err.h>       // err(), errx()
#include "defs.h"     // student_t, MAX_CHARS_NIF, struct options, action_t

#define MAXLEN_LINE_FILE 255  // Máxima longitud de línea de texto en el fichero de entrada

/**
 * Imprime el contenido de un fichero de texto de estudiantes
 * (opción -p), reusando la implementación de la Parte A.
 */
int print_text_file(char *path) {
    FILE *in = fopen(path, "r");
    if (!in)
        err(2, "No se pudo abrir el fichero '%s'", path);

    char line[MAXLEN_LINE_FILE + 1];
    int entry = 0;

    // Leer y descartar primera línea (número de registros)
    if (fgets(line, sizeof(line), in) == NULL) {
        if (feof(in)) { fclose(in); return EXIT_SUCCESS; }
        err(3, "Error leyendo número de registros");
    }

    // Procesar el resto de líneas (cada estudiante)
    while (fgets(line, sizeof(line), in) != NULL) {
        // Eliminar '\n' final
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        student_t stu;
        char *rest = line, *token;

        // student_id
        token = strsep(&rest, ":");
        stu.student_id = atoi(token);

        // NIF (cadena fija)
        token = strsep(&rest, ":");
        strncpy(stu.NIF, token, MAX_CHARS_NIF);
        stu.NIF[MAX_CHARS_NIF] = '\0';

        // first_name (puntero dentro de `line`)
        token = strsep(&rest, ":");
        stu.first_name = token;

        // last_name
        token = strsep(&rest, ":");
        stu.last_name = token;

        // Imprimir mismo formato que en la Parte A
        printf("[Entry #%d]\n", entry++);
        printf("\tstudent_id=%d\n",   stu.student_id);
        printf("\tNIF=%s\n",          stu.NIF);
        printf("\tfirst_name=%s\n",   stu.first_name);
        printf("\tlast_name=%s\n",    stu.last_name);
    }

    fclose(in);
    return EXIT_SUCCESS;
}

/**
 * Lee registros de texto y los vuelca en un fichero binario:
 *  - Primer entero: número de registros
 *  - Para cada registro:
 *      int student_id;
 *      char NIF[...] + '\0';
 *      char first_name[...] + '\0';
 *      char last_name[...] + '\0';
 */
int write_binary_file(char *input_file, char *output_file) {
    FILE *in  = fopen(input_file,  "r");
    FILE *out = fopen(output_file, "wb");
    if (!in)  err(2, "No se pudo abrir '%s'", input_file);
    if (!out) err(3, "No se pudo crear '%s'", output_file);

    char line[MAXLEN_LINE_FILE + 1];
    int total;

    // 1) Leer total de registros desde la primera línea
    if (fgets(line, sizeof(line), in) == NULL)
        err(4, "Error leyendo número de registros");
    total = atoi(line);

    // Escribir total en binario
    if (fwrite(&total, sizeof(int), 1, out) < 1)
        err(5, "Error escribiendo el número de registros");

    int entry = 0;
    // 2) Para cada línea: parsear y volcar campos
    while (fgets(line, sizeof(line), in) != NULL) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        student_t stu;
        char *rest = line, *token;

        // student_id
        token = strsep(&rest, ":");
        stu.student_id = atoi(token);
        // NIF
        token = strsep(&rest, ":");
        strncpy(stu.NIF, token, MAX_CHARS_NIF);
        stu.NIF[MAX_CHARS_NIF] = '\0';
        // first_name y last_name
        char *fn = strsep(&rest, ":");
        char *ln = strsep(&rest, ":");

        // a) Escribir student_id
        if (fwrite(&stu.student_id, sizeof(int), 1, out) < 1)
            err(6, "Error escribiendo student_id[%d]", entry);
        // b) Escribir NIF con '\0'
        size_t len = strlen(stu.NIF) + 1;
        if (fwrite(stu.NIF, 1, len, out) < len)
            err(7, "Error escribiendo NIF[%d]", entry);
        // c) first_name
        len = strlen(fn) + 1;
        if (fwrite(fn, 1, len, out) < len)
            err(8, "Error escribiendo first_name[%d]", entry);
        // d) last_name
        len = strlen(ln) + 1;
        if (fwrite(ln, 1, len, out) < len)
            err(9, "Error escribiendo last_name[%d]", entry);

        entry++;
    }

    fclose(in);
    fclose(out);

    // Mensaje final de éxito
    printf("%d student records written successfully to binary file %s\n",
           total, output_file);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    struct options opt = { .input_file = NULL,
                           .output_file = NULL,
                           .action      = NONE_ACT };
    int c;

    // Opciones: -h, -i <file>, -p, -o <output_file>
    while ((c = getopt(argc, argv, "hi:po:")) != -1) {
        switch (c) {
            case 'h':
                fprintf(stderr,
        "Usage: %s [ -h | -i file | -p | -o output_file ]\n",
                        argv[0]);
                exit(EXIT_SUCCESS);
            case 'i': opt.input_file  = optarg;             break;
            case 'p': opt.action      = PRINT_TEXT_ACT;     break;
            case 'o': opt.output_file = optarg;
                      opt.action      = WRITE_BINARY_ACT;  break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    // Validar argumentos
    if (opt.input_file == NULL)
        errx(EXIT_FAILURE, "Debe especificar -i <input_file>");
    if (opt.action == WRITE_BINARY_ACT) {
        if (opt.output_file == NULL)
            errx(EXIT_FAILURE, "Debe especificar -o <output_file>");
        return write_binary_file(opt.input_file, opt.output_file);
    }
    else if (opt.action == PRINT_TEXT_ACT) {
        return print_text_file(opt.input_file);
    }
    else {
        errx(EXIT_FAILURE, "Debe indicar -p o -o en la invocación");
    }
}
