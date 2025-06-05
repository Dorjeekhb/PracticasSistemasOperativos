/*
 * run_commands.c
 *
 * Ejecuta comandos pasados con -x o leídos de un fichero con -s, de forma
 * secuencial o paralela (-b), esperando a su terminación e informando de PID
 * y status.
 *
 * Uso:
 *   ./run_commands [-x "comando"] [-s fichero] [-b]
 *
 *   -x <comando>  Ejecuta un único comando y espera a que termine.
 *   -s <fichero>  Lee el fichero línea a línea, cada línea es un comando.
 *                 Por defecto: secuencial (espera a cada uno antes de lanzar el siguiente).
 *   -b            Solo con -s: lanza todos los comandos sin esperar, y luego
 *                 informa cada vez que uno termina.
 *
 * Ejemplos:
 *   ./run_commands -x "ls -l /"
 *   ./run_commands -s comandos.txt
 *   ./run_commands -b -s comandos.txt
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>     // getopt, fork, execvp
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CMDS 1024
#define LINEBUF 1024

/**
 * parse_command:
 *   Divide la línea cmd en tokens separados por espacios, construye argv[]
 *   terminado en NULL y devuelve también la cuenta de argumentos en *argc.
 */
char **parse_command(const char *cmd, int *argc) {
    size_t argv_size = 10;
    char **argv = malloc(argv_size * sizeof(char *));
    if (!argv) { perror("malloc"); exit(EXIT_FAILURE); }
    int count = 0;
    const char *start = cmd;
    while (*start && isspace((unsigned char)*start)) start++;
    while (*start) {
        if (count >= (int)argv_size - 1) {
            argv_size *= 2;
            argv = realloc(argv, argv_size * sizeof(char *));
            if (!argv) { perror("realloc"); exit(EXIT_FAILURE); }
        }
        const char *end = start;
        while (*end && !isspace((unsigned char)*end)) end++;
        size_t len = end - start;
        argv[count] = malloc(len + 1);
        if (!argv[count]) { perror("malloc"); exit(EXIT_FAILURE); }
        memcpy(argv[count], start, len);
        argv[count][len] = '\0';
        count++;
        start = end;
        while (*start && isspace((unsigned char)*start)) start++;
    }
    argv[count] = NULL;
    *argc = count;
    return argv;
}

/**
 * launch_command:
 *   Crea un hijo con fork(), y en el hijo hace execvp(argv[0], argv).
 *   El padre retorna inmediatamente el PID del hijo.
 */
pid_t launch_command(char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        // hijo
        execvp(argv[0], argv);
        // si execvp falla:
        fprintf(stderr, "execvp('%s') failed: %s\n",
                argv[0], strerror(errno));
        _exit(127);
    }
    // padre
    return pid;
}

int main(int argc, char *argv[]) {
    int opt;
    char *opt_x = NULL;
    char *opt_s = NULL;
    int  opt_b = 0;

    // 1) Parseo de opciones
    while ((opt = getopt(argc, argv, "x:s:b")) != -1) {
        switch (opt) {
        case 'x':
            opt_x = optarg;
            break;
        case 's':
            opt_s = optarg;
            break;
        case 'b':
            opt_b = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-x cmd] [-s file] [-b]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    // Validación mínima
    if (!opt_x && !opt_s) {
        fprintf(stderr, "Debe usar -x o -s\n");
        exit(EXIT_FAILURE);
    }
    if (opt_b && !opt_s) {
        fprintf(stderr, "-b solo tiene sentido con -s\n");
        exit(EXIT_FAILURE);
    }

    // 2) Caso -x: ejecutar un único comando y esperar
    if (opt_x) {
        int cargc;
        char **cargv = parse_command(opt_x, &cargc);
        pid_t pid = launch_command(cargv);
        if (pid < 0) exit(EXIT_FAILURE);
        int status;
        waitpid(pid, &status, 0);
        // liberamos memoria
        for (int i = 0; i < cargc; i++) free(cargv[i]);
        free(cargv);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }

    // 3) Caso -s: leer fichero línea a línea
    FILE *fp = fopen(opt_s, "r");
    if (!fp) {
        fprintf(stderr, "No se pudo abrir '%s': %s\n", opt_s, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!opt_b) {
        // Modo secuencial
        char line[LINEBUF];
        int cmdno = 0;
        while (fgets(line, sizeof(line), fp)) {
            // quitamos '\n'
            line[strcspn(line, "\n")] = '\0';
            printf("@@ Running command #%d: %s\n", cmdno, line);

            int cargc;
            char **cargv = parse_command(line, &cargc);
            pid_t pid = launch_command(cargv);
            int status;
            waitpid(pid, &status, 0);
            printf("@@ Command #%d terminated (pid: %d, status: %d)\n",
                   cmdno, pid, status);

            for (int i = 0; i < cargc; i++) free(cargv[i]);
            free(cargv);
            cmdno++;
        }
    } else {
        // Modo paralelo (-b)
        pid_t pids[MAX_CMDS];
        int    count = 0;
        char * lines[MAX_CMDS];

        // Lanzamos todos los comandos sin esperar
        char buf[LINEBUF];
        while (fgets(buf, sizeof(buf), fp) && count < MAX_CMDS) {
            buf[strcspn(buf, "\n")] = '\0';
            lines[count] = strdup(buf);
            printf("@@ Running command #%d: %s\n", count, buf);

            int cargc;
            char **cargv = parse_command(buf, &cargc);
            pid_t pid = launch_command(cargv);
            pids[count] = pid;

            // liberamos argv pero no el string principal
            for (int i = 0; i < cargc; i++) free(cargv[i]);
            free(cargv);

            count++;
        }

        // Ahora esperamos a que terminen todos, en el orden que sea
        int finished = 0;
        while (finished < count) {
            int status;
            pid_t pid = wait(&status);
            if (pid < 0) {
                perror("wait");
                break;
            }
            // Buscar índice de este pid
            for (int i = 0; i < count; i++) {
                if (pids[i] == pid) {
                    printf("@@ Command #%d terminated (pid: %d, status: %d)\n",
                           i, pid, status);
                    break;
                }
            }
            finished++;
        }
        // liberamos líneas
        for (int i = 0; i < count; i++)
            free(lines[i]);
    }

    fclose(fp);
    return EXIT_SUCCESS;
}
