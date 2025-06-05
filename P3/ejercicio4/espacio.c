/*
 * espacio.c
 *
 * Calcula el espacio ocupado (en kilobytes) por uno o varios ficheros o
 * directorios, recursivamente. Para directorios, suma blocks de todos sus
 * contenidos.
 *
 * Uso:
 *   ./espacio <ruta1> [<ruta2> ...]
 *
 * Para cada argumento:
 *   - Llama a lstat() para obtener st_blocks (512 B blocks reservados).
 *   - Si es un fichero regular o enlace, acumula st_blocks.
 *   - Si es un directorio, abre con opendir() y recorre entries con readdir(),
 *     ignora "." y ".." y aplica recursión.
 *   - Convierte total de 512-byte blocks a kilobytes redondeando hacia arriba:
 *       kilobytes = ceil((blocks * 512) / 1024) = (blocks + 1) / 2
 *
 * Salida:
 *   Una línea por argumento: "<kilobytes>K <ruta>"
 *
 * Páginas de manual:
 *   man 2 lstat, struct stat
 *   man 2 opendir, readdir, closedir
 *   man 2 strerror, errno
 *
 * Autora: Dorjee
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/**
 * get_size_dir:
 *   Añade a *blocks el número de bloques de 512 B de todos los ficheros
 *   contenidos recursivamente en el directorio 'dirpath'.
 *   Incluye los bloques del propio directorio.
 *
 * Parámetros:
 *   dirpath: ruta al directorio
 *   blocks:  puntero al acumulador de bloques (se suman aquí)
 *
 * Retorno:
 *   0 en éxito, -1 en error (mensaje impreso con perror o fprintf).
 */
int get_size_dir(const char *dirpath, size_t *blocks) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "Error abriendo directorio '%s': %s\n",
                dirpath, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    char path[PATH_MAX];

    // Recorremos cada entrada del directorio
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        // Ignorar "." y ".." para evitar recursión infinita
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        // Construir ruta completa: dirpath + '/' + name
        snprintf(path, sizeof(path), "%s/%s", dirpath, name);

        // Obtener tamaño de esta ruta (file o subdirectorio)
        struct stat st;
        if (lstat(path, &st) != 0) {
            fprintf(stderr, "Error en lstat('%s'): %s\n",
                    path, strerror(errno));
            closedir(dir);
            return -1;
        }

        // Acumular bloques del propio fichero o enlace
        *blocks += st.st_blocks;

        // Si es un subdirectorio, recursión
        if (S_ISDIR(st.st_mode)) {
            if (get_size_dir(path, blocks) != 0) {
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

/**
 * get_size:
 *   Calcula el número total de bloques de 512 B reservados para 'path'.
 *   - Si es un directorio, incluye recursivamente su contenido.
 *   - Si es fichero normal o enlace, toma st_blocks.
 *
 * Parámetros:
 *   path:   ruta al fichero o directorio
 *   blocks: puntero donde se almacenan los bloques (inicializado a 0)
 *
 * Retorno:
 *   0 en éxito, -1 en error.
 */
int get_size(const char *path, size_t *blocks) {
    struct stat st;
    // lstat para no seguir enlaces simbólicos
    if (lstat(path, &st) != 0) {
        fprintf(stderr, "Error en lstat('%s'): %s\n",
                path, strerror(errno));
        return -1;
    }

    // Empezamos sumando los bloques del propio path
    *blocks = st.st_blocks;

    // Si es directorio, procesar recursivamente su contenido
    if (S_ISDIR(st.st_mode)) {
        if (get_size_dir(path, blocks) != 0)
            return -1;
    }

    return 0;
}

/**
 * main: procesa la lista de argumentos y muestra tamaño en KB.
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fichero_o_directorio> [<otro> ...]\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    // Para cada argumento: calcular y mostrar tamaño
    for (int i = 1; i < argc; ++i) {
        const char *path = argv[i];
        size_t blocks = 0;

        if (get_size(path, &blocks) != 0) {
            // En caso de error, seguimos con el siguiente
            continue;
        }

        // Convertir bloques de 512B a kilobytes (ceil)
        size_t kbytes = (blocks + 1) / 2;
        printf("%zuK %s\n", kbytes, path);
    }

    return EXIT_SUCCESS;
}
