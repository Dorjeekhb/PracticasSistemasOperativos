/*
 * disco.c
 *
 * Simulación de control de aforo en una discoteca con prioridad a VIPs.
 *
 * Premisas:
 *   - Aforo máximo: CAPACITY clientes dentro simultáneamente.
 *   - Dos tipos de clientes: VIP (isvip==1) y normales (isvip==0).
 *   - Si hay hueco y no hay VIP esperando, entra el siguiente normal.
 *   - Si hay VIP esperando, éstos siempre tienen preferencia sobre los normales.
 *   - La entrada es FIFO dentro de cada categoría: VIPs se atienden en orden
 *     de llegada, luego normales en orden de llegada.
 *   - Se crean M hilos: cada hilo representa un cliente que entra, baila y sale.
 *
 * Sincronización:
 *   - Mutex para proteger el acceso a variables globales de estado.
 *   - Dos variables de condición: una para VIPs, otra para normales.
 *   - Contadores de cuántos están dentro y cuántos esperan de cada tipo.
 *
 * Compilación:
 *   gcc -Wall -Wextra -std=gnu99 -pthread -o disco disco.c
 */

#include <stdio.h>      // printf, perror
#include <stdlib.h>     // exit, EXIT_FAILURE, malloc, free, srand, rand
#include <unistd.h>     // sleep, getpid
#include <pthread.h>    // pthread_t, pthread_create, pthread_join, mutex, cond

// CAPACITY: número máximo de clientes dentro a la vez.
// Ajusta este valor según las reglas de la discoteca.
#define CAPACITY 5

// Macro auxiliar para imprimir " vip " o "not vip"
#define VIPSTR(vip) ((vip) ? "  vip  " : "not vip")

// --------------------------------------------------
// Estructura de argumentos que recibe cada hilo:
//   - id:     identificador único del cliente (0..M-1)
//   - isvip:  1 si es VIP, 0 si es cliente normal
typedef struct {
    int id;       // identificador de cliente
    int isvip;    // bandera VIP (1) o normal (0)
} client_arg_t;

// --------------------------------------------------
// Variables de estado global protegidas por mutex:
// --------------------------------------------------

// inside_count: cuántos clientes están actualmente dentro de la discoteca.
static int inside_count = 0;

// waiting_vip:   cuántos clientes VIP están esperando fuera (bloqueados).
static int waiting_vip = 0;

// waiting_norm: cuántos clientes normales están esperando fuera.
static int waiting_normal = 0;

// Mutex para proteger todas las variables de estado anteriores.
// Toda modificación/lectura de inside_count, waiting_vip o waiting_normal
// debe hacerse dentro de pthread_mutex_lock/mutex_unlock.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Condición para despertar clientes VIP.
// Cuando haya hueco y no haya aún VIPs por atender, se señaliza vip_cond.
static pthread_cond_t vip_cond = PTHREAD_COND_INITIALIZER;

// Condición para despertar clientes normales.
// Se usa cuando no hay VIP esperando y hay hueco.
static pthread_cond_t normal_cond = PTHREAD_COND_INITIALIZER;

// --------------------------------------------------
// enter_vip_client:
//   Función que llama un hilo VIP para entrar en la disco.
//   Bloquea hasta que haya hueco (< CAPACITY).
// --------------------------------------------------
void enter_vip_client(int id) {
    // 1) Bloqueamos el mutex para leer/modificar estado compartido.
    pthread_mutex_lock(&mutex);

    // 2) Incrementamos el contador de VIP esperando.
    waiting_vip++;
    printf("Client %2d (%s) wants to enter (waiting_vip=%d)\n",
           id, VIPSTR(1), waiting_vip);

    // 3) Mientras no haya hueco (inside_count >= CAPACITY), esperamos.
    //    Nota: usamos pthread_cond_wait, que desbloquea el mutex y duerme.
    //    Al volver de la espera, retranca el mutex antes de seguir.
    while (inside_count >= CAPACITY) {
        pthread_cond_wait(&vip_cond, &mutex);
    }

    // 4) Hemos salido del bucle: hay hueco y soy el siguiente VIP.
    //    Decremento VIP esperando e incrementamos ocupación.
    waiting_vip--;
    inside_count++;
    printf("Client %2d (%s) entering, occupancy=%d\n",
           id, VIPSTR(1), inside_count);

    // 5) Desbloqueo el mutex para liberar la sección crítica.
    pthread_mutex_unlock(&mutex);
}

// --------------------------------------------------
// enter_normal_client:
//   Función que llama un hilo normal para entrar.
//   Bloquea si no hay hueco oSI hay VIPs esperando.
// --------------------------------------------------
void enter_normal_client(int id) {
    pthread_mutex_lock(&mutex);

    waiting_normal++;
    printf("Client %2d (%s) wants to enter (waiting_norm=%d)\n",
           id, VIPSTR(0), waiting_normal);

    // Condición de espera para clientes normales:
    //  - no debe superarse CAPACITY
    //  - no debe haber VIPs esperando (donde waiting_vip>0)
    while (inside_count >= CAPACITY || waiting_vip > 0) {
        pthread_cond_wait(&normal_cond, &mutex);
    }

    // Tras salir del bucle, se puede entrar:
    waiting_normal--;
    inside_count++;
    printf("Client %2d (%s) entering, occupancy=%d\n",
           id, VIPSTR(0), inside_count);

    pthread_mutex_unlock(&mutex);
}

// --------------------------------------------------
// disco_exit:
//   Llamada al salir un cliente (VIP o normal).
//   Reduce inside_count y despierta a clientes esperando
//   (primero a VIPs si los hay, sino a normales).
// --------------------------------------------------
void disco_exit(int id, int isvip) {
    // 1) Bloquear mutex para modificar shared state.
    pthread_mutex_lock(&mutex);

    inside_count--;
    printf("Client %2d (%s) leaving, occupancy=%d\n",
           id, VIPSTR(isvip), inside_count);

    // 2) Si hay VIP esperando, señalizamos vip_cond.
    //    Esto despierta exactamente un VIP bloqueado.
    if (waiting_vip > 0) {
        pthread_cond_signal(&vip_cond);
    }
    // 3) Si no hay VIP esperando, pero sí normales, señalizamos normal_cond.
    else if (waiting_normal > 0) {
        pthread_cond_signal(&normal_cond);
    }

    // 4) Liberamos el mutex.
    pthread_mutex_unlock(&mutex);
}

// --------------------------------------------------
// dance:
//   Simula que el cliente baila un tiempo aleatorio.
//   Solo imprimimos un mensaje y dormimos entre 1 y 3 segundos.
// --------------------------------------------------
void dance(int id, int isvip) {
    printf("Client %2d (%s) dancing in disco\n",
           id, VIPSTR(isvip));
    // sleep aleatorio para simular duración del baile
    sleep((rand() % 3) + 1);
}

// --------------------------------------------------
// client:
//   Función principal que ejecuta cada hilo/cliente.
//   - lee su id e isvip de la estructura arg.
//   - llama a la función de entrada adecuada.
//   - baila y luego sale.
// --------------------------------------------------
void *client(void *arg) {
    client_arg_t *ca = (client_arg_t *)arg;

    int id    = ca->id;
    int isvip = ca->isvip;

    // Liberamos la memoria dinámica ocupada por arg
    free(ca);

    // Lógica VIP vs normal
    if (isvip) {
        enter_vip_client(id);
    } else {
        enter_normal_client(id);
    }

    // Ahora el cliente está dentro. Simulamos baile...
    dance(id, isvip);

    // Finalmente sale
    disco_exit(id, isvip);

    return NULL;
}

// --------------------------------------------------
// main:
//   - Lee un fichero de entrada con M y luego M líneas 0/1.
//   - Para cada línea crea un hilo client(...) con id e isvip.
//   - Espera a que todos los hilos terminen.
// --------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 1) Abrir el fichero de configuración
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen input_file");
        return EXIT_FAILURE;
    }

    // 2) Leer el número de clientes M
    int M;
    if (fscanf(fp, "%d\n", &M) != 1) {
        fprintf(stderr, "Error: bad format in %s\n", argv[1]);
        fclose(fp);
        return EXIT_FAILURE;
    }

    // 3) Reservamos array para los pthread_t
    pthread_t *threads = malloc(sizeof(pthread_t) * M);
    if (!threads) {
        perror("malloc threads");
        fclose(fp);
        return EXIT_FAILURE;
    }

    // Inicializamos la semilla aleatoria para sleep()
    srand(getpid());

    // 4) Creación de hilos
    for (int i = 0; i < M; i++) {
        int vipflag;
        // leemos 0 (normal) o 1 (VIP)
        if (fscanf(fp, "%d\n", &vipflag) != 1) {
            fprintf(stderr, "Error: missing entry %d in %s\n", i, argv[1]);
            free(threads);
            fclose(fp);
            return EXIT_FAILURE;
        }
        // Preparamos arg dinámico para el hilo
        client_arg_t *ca = malloc(sizeof(*ca));
        if (!ca) {
            perror("malloc client_arg");
            free(threads);
            fclose(fp);
            return EXIT_FAILURE;
        }
        ca->id    = i;
        ca->isvip = vipflag ? 1 : 0;

        // Creamos el hilo
        if (pthread_create(&threads[i], NULL, client, ca) != 0) {
            perror("pthread_create");
            free(ca);
            // continuamos intentando crear los demás hilos
        }
    }
    // Cerramos el fichero de configuración
    fclose(fp);

    // 5) Esperamos a que todos los hilos terminen
    for (int i = 0; i < M; i++) {
        pthread_join(threads[i], NULL);
    }

    // 6) Liberamos memoria y salimos
    free(threads);
    return EXIT_SUCCESS;
}
