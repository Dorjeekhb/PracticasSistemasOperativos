#include <stdio.h>     // fopen, fgets, printf, fprintf, stderr, ftell, fseek, fgetc, feof
#include <stdlib.h>    // EXIT_SUCCESS, EXIT_FAILURE, atoi, malloc, free
#include <unistd.h>    // getopt
#include <string.h>    // strchr, strsep, strncpy, strlen
#include <err.h>       // err(), errx()
#include "defs.h"     // student_t, MAX_CHARS_NIF, struct options, action_t

#define MAXLEN_LINE_FILE 255  // Máxima longitud de línea de texto en el fichero de entrada

/**
 * Parte A: imprimir fichero de texto de estudiantes (-p)
 */
int print_text_file(char *path) {
    FILE *in = fopen(path, "r");
    if (!in) err(2, "No se pudo abrir el fichero '%s'", path);

    char line[MAXLEN_LINE_FILE + 1];
    int entry = 0;

    // Leer y descartar primera línea (número de registros)
    if (fgets(line, sizeof(line), in) == NULL) {
        if (feof(in)) { fclose(in); return EXIT_SUCCESS; }
        err(3, "Error leyendo número de registros");
    }

    while (fgets(line, sizeof(line), in) != NULL) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        student_t stu;
        char *rest = line, *token;

        token = strsep(&rest, ":");
        stu.student_id = atoi(token);

        token = strsep(&rest, ":");
        strncpy(stu.NIF, token, MAX_CHARS_NIF);
        stu.NIF[MAX_CHARS_NIF] = '\0';

        token = strsep(&rest, ":");
        stu.first_name = token;

        token = strsep(&rest, ":");
        stu.last_name = token;

        printf("[Entry #%d]\n", entry++);
        printf("\tstudent_id=%d\n", stu.student_id);
        printf("\tNIF=%s\n", stu.NIF);
        printf("\tfirst_name=%s\n", stu.first_name);
        printf("\tlast_name=%s\n", stu.last_name);
    }

    fclose(in);
    return EXIT_SUCCESS;
}

/**
 * Parte B: escribir fichero binario (-o)
 */
int write_binary_file(char *input_file, char *output_file) {
    FILE *in  = fopen(input_file,  "r");
    FILE *out = fopen(output_file, "wb");
    if (!in)  err(2, "No se pudo abrir '%s'", input_file);
    if (!out) err(3, "No se pudo crear '%s'", output_file);

    char line[MAXLEN_LINE_FILE + 1];
    int total;

    if (fgets(line, sizeof(line), in) == NULL)
        err(4, "Error leyendo número de registros");
    total = atoi(line);

    if (fwrite(&total, sizeof(int), 1, out) < 1)
        err(5, "Error escribiendo el número de registros");

    int entry = 0;
    while (fgets(line, sizeof(line), in) != NULL) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        student_t stu;
        char *rest = line, *token;
        token = strsep(&rest, ":"); stu.student_id = atoi(token);
        token = strsep(&rest, ":"); strncpy(stu.NIF, token, MAX_CHARS_NIF); stu.NIF[MAX_CHARS_NIF] = '\0';
        char *fn = strsep(&rest, ":");
        char *ln = strsep(&rest, ":");

        if (fwrite(&stu.student_id, sizeof(int), 1, out) < 1)
            err(6, "Error escribiendo student_id[%d]", entry);
        size_t len = strlen(stu.NIF) + 1;
        if (fwrite(stu.NIF, 1, len, out) < len)
            err(7, "Error escribiendo NIF[%d]", entry);
        len = strlen(fn) + 1;
        if (fwrite(fn, 1, len, out) < len)
            err(8, "Error escribiendo first_name[%d]", entry);
        len = strlen(ln) + 1;
        if (fwrite(ln, 1, len, out) < len)
            err(9, "Error escribiendo last_name[%d]", entry);

        entry++;
    }

    fclose(in);
    fclose(out);
    printf("%d student records written successfully to binary file %s\n", total, output_file);
    return EXIT_SUCCESS;
}

/**
 * Parte C: leer fichero binario (-b)
 * Función auxiliar loadstr(): lee cadena terminada en '\0'
 */
static char *loadstr(FILE *file) {
    long start = ftell(file);
    if (start < 0) err(10, "ftell() falló");

    int ch;
    size_t len = 0;
    while ((ch = fgetc(file)) != EOF) {
        len++;
        if (ch == '\0') break;
    }
    if (ch == EOF) err(11, "EOF inesperado al leer cadena");

    if (fseek(file, start, SEEK_SET) != 0)
        err(12, "fseek() falló");

    char *buf = malloc(len);
    if (!buf) err(13, "malloc() falló");
    if (fread(buf, 1, len, file) < len)
        err(14, "fread() falló");
    return buf;
}

int print_binary_file(char *path) {
    FILE *in = fopen(path, "rb");
    if (!in) err(2, "No se pudo abrir el fichero binario '%s'", path);

    int total;
    if (fread(&total, sizeof(int), 1, in) < 1)
        err(3, "Error leyendo el número de registros");

    for (int entry = 0; entry < total; entry++) {
        student_t stu;
        if (fread(&stu.student_id, sizeof(int), 1, in) < 1)
            err(4, "Error leyendo student_id[%d]", entry);

        char *tmp = loadstr(in);
        strncpy(stu.NIF, tmp, MAX_CHARS_NIF);
        stu.NIF[MAX_CHARS_NIF] = '\0';
        free(tmp);

        stu.first_name = loadstr(in);
        stu.last_name  = loadstr(in);

        printf("[Entry #%d]\n", entry);
        printf("\tstudent_id=%d\n", stu.student_id);
        printf("\tNIF=%s\n", stu.NIF);
        printf("\tfirst_name=%s\n", stu.first_name);
        printf("\tlast_name=%s\n", stu.last_name);

        free(stu.first_name);
        free(stu.last_name);
    }

    fclose(in);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    struct options opt = { .input_file = NULL,
                           .output_file = NULL,
                           .action      = NONE_ACT };
    int c;

    while ((c = getopt(argc, argv, "hi:po:b")) != -1) {
        switch (c) {
            case 'h':
                fprintf(stderr, "Usage: %s [ -h | -i file | -p | -o output_file | -b ]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 'i': opt.input_file  = optarg;             break;
            case 'p': opt.action      = PRINT_TEXT_ACT;     break;
            case 'o': opt.output_file = optarg;
                      opt.action      = WRITE_BINARY_ACT;  break;
            case 'b': opt.action      = PRINT_BINARY_ACT;  break;
            default:  exit(EXIT_FAILURE);
        }
    }

    if (!opt.input_file)
        errx(EXIT_FAILURE, "Debe especificar -i <input_file>");

    switch (opt.action) {
        case PRINT_TEXT_ACT:
            return print_text_file(opt.input_file);
        case WRITE_BINARY_ACT:
            if (!opt.output_file)
                errx(EXIT_FAILURE, "Debe especificar -o <output_file>");
            return write_binary_file(opt.input_file, opt.output_file);
        case PRINT_BINARY_ACT:
            return print_binary_file(opt.input_file);
        default:
            errx(EXIT_FAILURE, "Debe indicar -p, -o o -b");
    }
    return EXIT_SUCCESS;
}
