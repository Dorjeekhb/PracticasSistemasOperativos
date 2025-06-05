#!/usr/bin/env bash
#
# prepara_ficheros.sh
#
# Crea un directorio (limpia si ya existía),
# genera los ficheros y enlaces solicitados,
# y muestra sus atributos con stat.
#
# Uso:
#   ./prepara_ficheros.sh <directorio>

set -euo pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <directorio>"
  exit 1
fi

DIR=$1

# 1) (Re)crear el directorio
if [ -d "$DIR" ]; then
  rm -rf "${DIR:?}/"*
else
  mkdir -p "$DIR"
fi

cd "$DIR"

# 2) Crear los objetos
mkdir subdir                       # subdir (directorio)
touch fichero1                     # fichero1 (vacío)
# fichero2: 10 caracteres, sin salto final:
echo -n 'abcdefghij' > fichero2    # fichero2 (10 bytes)
ln -s fichero2 enlaceS             # enlaceS (símbolico a fichero2)
ln fichero2 enlaceH                # enlaceH (hard link a fichero2)

# 3) Mostrar atributos con stat
echo "=== Atributos de todos los elementos en $DIR ==="
for f in subdir fichero1 fichero2 enlaceS enlaceH; do
  stat "$f"
  echo
done
