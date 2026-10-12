#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOSTFILE="${1:-$ROOT/hosts.txt}"
if [[ $# -gt 0 ]]; then shift; fi

PROCESSES="${PROCESSES:-5}"
THREADS="${OMP_NUM_THREADS:-2}"
MPIRUN="${MPIRUN:-mpirun}"
APP="${APP:-$ROOT/busqueda_hibrida}"
LOG_DIR="${LOG_DIR:-$ROOT/logs/cluster}"

if [[ ! -f "$HOSTFILE" ]]; then
  echo "Falta el hostfile: $HOSTFILE" >&2
  exit 1
fi

make -C "$ROOT"
exec env OMP_NUM_THREADS="$THREADS" "$MPIRUN" \
  --hostfile "$HOSTFILE" -np "$PROCESSES" -x OMP_NUM_THREADS \
  "$APP" --logs "$LOG_DIR" "$@"
