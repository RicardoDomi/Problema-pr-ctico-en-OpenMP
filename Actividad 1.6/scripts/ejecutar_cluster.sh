#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOSTFILE="${1:-$ROOT/hosts.txt}"
THREADS="${OMP_NUM_THREADS:-2}"
APP="${APP:-$ROOT/fuerza_bruta}"
LOG_DIR="${LOG_DIR:-$ROOT/logs/red}"

if [[ ! -f "$HOSTFILE" ]]; then
  echo "Falta el hostfile: $HOSTFILE" >&2
  echo "Copia hosts.example como hosts.txt y agrega las IP o nombres reales." >&2
  exit 1
fi

make -C "$ROOT"
OMP_NUM_THREADS="$THREADS" mpirun \
  --hostfile "$HOSTFILE" -np 5 -x OMP_NUM_THREADS \
  "$APP" --logs "$LOG_DIR"
