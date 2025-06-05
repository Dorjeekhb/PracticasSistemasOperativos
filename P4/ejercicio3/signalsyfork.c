/**
 * signalsyfork.c
 *
 * Crea un hijo que ejecuta el comando pasado como argumentos. El padre
 * programa una alarma a 5 segundos; cuando venza, su manejador envía
 * SIGKILL al hijo. Al final espera al hijo e informa si terminó normalmente
 * o por señal. Además, el padre ignora SIGINT.
 *
 * Uso:
 *   ./signalsyfork <comando> [args...]
 *
 * Ejemplos:
 *   ./signalsyfork ls -l
 *   ./signalsyfork sleep 10
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp, alarm, pause
#include <signal.h>     // sigaction, sigemptyset, kill, SIGKILL, SIGINT
#include <sys/wait.h>   // waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <string.h>
#include <errno.h>

static pid_t child_pid = -1;

/* Manejador de SIGALRM: mata al hijo con SIGKILL */
void on_alarm(int sig) {
    if (child_pid > 0) {
        kill(child_pid, SIGKILL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <comando> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 1) Instalar manejador de SIGALRM */
    struct sigaction sa;
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction(SIGALRM)");
        return EXIT_FAILURE;
    }

    /* 2) Ignorar SIGINT en el padre */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("signal(SIGINT)");
        return EXIT_FAILURE;
    }

    /* 3) Fork y execvp en el hijo */
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child_pid == 0) {
        /* Código del hijo: restaura el manejo por defecto de SIGINT */
        signal(SIGINT, SIG_DFL);
        /* Ejecuta el comando */
        execvp(argv[1], &argv[1]);
        /* Si exec falla */
        fprintf(stderr, "execvp(%s): %s\n", argv[1], strerror(errno));
        _exit(127);
    }

    /* 4) En el padre: programar la alarma a 5 segundos */
    alarm(5);

    /* 5) Esperar al hijo */
    int status;
    pid_t w = waitpid(child_pid, &status, 0);
    if (w == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }

    /* 6) Informar la causa de terminación */
    if (WIFEXITED(status)) {
        printf("Child %d exited normally, status=%d\n",
               child_pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child %d killed by signal %d (%s)\n",
               child_pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
    } else {
        printf("Child %d terminated abnormally (status=0x%x)\n",
               child_pid, status);
    }

    return EXIT_SUCCESS;
}
