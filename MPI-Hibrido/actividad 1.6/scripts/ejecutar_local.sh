#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THREADS="${OMP_NUM_THREADS:-2}"

make -C "$ROOT"
OMP_NUM_THREADS="$THREADS" mpirun -np 5 \
  "$ROOT/fuerza_bruta" --logs "$ROOT/logs/local"
