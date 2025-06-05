#include <stdio.h>
#include <unistd.h>  // getopt
#include <stdlib.h>  // EXIT_SUCCESS, EXIT_FAILURE, atoi
#include <string.h>  // strchr, strsep, strncpy
#include <err.h>     // err(), errx()
#include "defs.h"

#define MAXLEN_LINE_FILE 100

/**
 * Lee un fichero de texto con registros "id:NIF:first:last" y
 * muestra cada entrada en formato amigable.
 */
int print_text_file(char *path) {
    FILE *in = fopen(path, "r");
    if (in == NULL)
        err(2, "No se pudo abrir el fichero '%s'", path);

    char line[MAXLEN_LINE_FILE + 1];
    
    // 1) Leer y descartar la primera línea (número de registros)
    if (fgets(line, sizeof(line), in) == NULL) {
        if (feof(in)) {
            fclose(in);
            return EXIT_SUCCESS;
        }
        err(3, "Error leyendo número de registros");
    }

    // 2) Leer cada registro y procesar
    int entry = 0;
    while (fgets(line, sizeof(line), in) != NULL) {
        // Eliminar posible '\n'
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        // Parsear campos separados por ':'
        student_t stu;
        char *rest = line;
        char *token;

        // student_id
        token = strsep(&rest, ":");
        stu.student_id = atoi(token);

        // NIF
        token = strsep(&rest, ":");
        strncpy(stu.NIF, token, MAX_CHARS_NIF);
        stu.NIF[MAX_CHARS_NIF] = '\0';

        // first_name
        token = strsep(&rest, ":");
        stu.first_name = token;

        // last_name
        token = strsep(&rest, ":");
        stu.last_name = token;

        // Imprimir registro
        printf("[Entry #%d]\n", entry++);
        printf("\tstudent_id=%d\n",   stu.student_id);
        printf("\tNIF=%s\n",          stu.NIF);
        printf("\tfirst_name=%s\n",   stu.first_name);
        printf("\tlast_name=%s\n",    stu.last_name);
    }

    fclose(in);
    return EXIT_SUCCESS;
}

int print_binary_file(char *path) {
    return 0;
}

int write_binary_file(char *in, char *out) {
    return 0;
}

int main(int argc, char *argv[]) {
    struct options opt;
    int c;

    // Inicializar opciones por defecto
    opt.input_file  = NULL;
    opt.output_file = NULL;
    opt.action      = NONE_ACT;

    // Procesar opciones -h, -i <file>, -p
    while ((c = getopt(argc, argv, "hi:p")) != -1) {
        switch (c) {
            case 'h':
                fprintf(stderr, "Usage: %s [ -h | -i <file> | -p ]\n", argv[0]);
                exit(EXIT_SUCCESS);
            case 'i':
                opt.input_file = optarg;
                break;
            case 'p':
                opt.action = PRINT_TEXT_ACT;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (opt.input_file == NULL)
        errx(EXIT_FAILURE, "Debe especificar -i <input_file>");
    if (opt.action != PRINT_TEXT_ACT)
        errx(EXIT_FAILURE, "Debe indicar la opción -p");

    return print_text_file(opt.input_file);
}
