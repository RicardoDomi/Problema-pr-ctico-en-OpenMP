#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROCESSES="${PROCESSES:-5}"
THREADS="${OMP_NUM_THREADS:-2}"
MPIRUN="${MPIRUN:-mpirun}"

make -C "$ROOT"
exec env OMP_NUM_THREADS="$THREADS" "$MPIRUN" -np "$PROCESSES" \
  "$ROOT/busqueda_hibrida" --logs "$ROOT/logs/local" "$@"
