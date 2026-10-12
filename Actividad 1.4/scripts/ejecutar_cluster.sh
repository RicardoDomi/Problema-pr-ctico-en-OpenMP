#!/usr/bin/env bash
set -euo pipefail

ACTIVIDAD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOSTFILE="${1:-hosts.txt}"
cd "$ACTIVIDAD_DIR"

if [[ ! -f "$HOSTFILE" ]]; then
  echo "No existe el hostfile: $HOSTFILE" >&2
  echo "Copia hosts.example como hosts.txt y coloca los hosts reales." >&2
  exit 1
fi

make
OMP_NUM_THREADS="${OMP_NUM_THREADS:-2}" \
  mpirun --hostfile "$HOSTFILE" --map-by slot --bind-to none -np 5 \
  ./cuenta_bancaria --logs logs/cluster
