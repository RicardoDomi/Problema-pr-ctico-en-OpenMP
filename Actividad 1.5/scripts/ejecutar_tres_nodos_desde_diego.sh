#!/usr/bin/env bash
set -euo pipefail

RUNNER="/Users/diegosilva12/mpi-stack/mpirun-tailscale"
DIEGO="/Users/diegosilva12/mpi-stack/actividades/actividad 1.5"
RICARDO="/Users/richard/mpi/actividades/actividad 1.5"
KRAATA="/home/kraata/mpi/actividades/actividad 1.5"

exec "$RUNNER" \
  --timeout 3600 \
  --oversubscribe \
  --bind-to none \
  --output tag \
  --mca btl_tcp_if_include 100.64.0.0/10 \
  -np 1 --host localhost -x OMP_NUM_THREADS=8 \
  "$DIEGO/busqueda_hibrida" "$@" --logs "$DIEGO/logs/cluster" \
  : \
  -np 2 --host 100.99.16.81:2 -x OMP_NUM_THREADS=6 \
  "$RICARDO/busqueda_hibrida" "$@" --logs "$RICARDO/logs/cluster" \
  : \
  -np 2 --host 100.72.80.61:2 -x OMP_NUM_THREADS=12 \
  "$KRAATA/busqueda_hibrida" "$@" --logs "$KRAATA/logs/cluster"
