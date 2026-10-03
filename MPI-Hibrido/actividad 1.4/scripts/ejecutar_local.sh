#!/usr/bin/env bash
set -euo pipefail

ACTIVIDAD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ACTIVIDAD_DIR"

make
OMP_NUM_THREADS="${OMP_NUM_THREADS:-2}" \
  mpirun --bind-to none -np 5 ./cuenta_bancaria --logs logs/local
