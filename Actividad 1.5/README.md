# Actividad 1.5 — búsquedas y ordenamiento híbridos

Aplicación C++ que combina MPI y OpenMP para trabajar con un arreglo dinámico,
comparar búsquedas locales y distribuidas, y medir sus tiempos de ejecución.

## Integrantes

- Castolo Gonzales Jorge Natanael
- Dominguez Alcala Ricardo
- Silva Montes Diego Eduardo

## Características

- Manejo de memoria dinámica mediante la clase `ArregloDinamico` y `new[]`.
- Generación paralela de datos con OpenMP.
- Merge Sort secuencial y Parallel Merge Sort con tareas OpenMP.
- Búsqueda secuencial por bloques y búsqueda binaria paralelizadas con OpenMP.
- Distribución del arreglo mediante `MPI_Scatterv`.
- Consolidación del menor índice global mediante `MPI_Reduce` con `MPI_MIN`.
- Medición separada de tiempo local, tiempo distribuido y comunicación.
- Validación automática de resultados locales frente a distribuidos.
- Logs independientes por equipo y rank MPI.
- Modo detallado para mostrar hilo, bloque, algoritmo y resultado.

No se utiliza `std::vector` como sustituto del arreglo dinámico.

## Estructura

```text
Actividad 1.5/
├── src/
│   ├── ArregloDinamico.cpp
│   ├── ArregloDinamico.h
│   ├── Busquedas.cpp
│   ├── Busquedas.h
│   ├── Registro.cpp
│   ├── Registro.h
│   └── main.cpp
├── scripts/
│   ├── ejecutar_local.sh
│   ├── ejecutar_cluster.sh
│   └── ejecutar_tres_nodos_desde_diego.sh
├── logs/
├── hosts.example
└── Makefile
```

## Requisitos

- Compilador compatible con C++17.
- Implementación de MPI con `mpic++` y `mpirun`.
- OpenMP. En macOS se requiere `libomp`.

## Compilación

```bash
make
```

Si se utiliza un compilador MPI instalado en otra ruta:

```bash
make CXX=/ruta/al/binario/mpic++
```

## Ejecución local

La siguiente orden utiliza cinco procesos MPI y dos hilos por proceso:

```bash
OMP_NUM_THREADS=2 make run
```

También puede utilizarse el script:

```bash
PROCESSES=5 OMP_NUM_THREADS=2 ./scripts/ejecutar_local.sh --n 1000000 --max 1000000
```

Prueba pequeña con trazabilidad por hilo:

```bash
PROCESSES=5 OMP_NUM_THREADS=2 ./scripts/ejecutar_local.sh \
  --n 40 --max 100 --target 42 --detallado
```

## Menú

1. Generar arreglo dinámico.
2. Ordenar arreglo con OpenMP.
3. Comparar búsqueda secuencial local y distribuida.
4. Comparar búsqueda binaria local y distribuida.
5. Mostrar tiempos, speedup y eficiencia.
6. Salir y liberar la memoria.

La búsqueda binaria requiere haber generado y ordenado previamente el arreglo.

## Argumentos

| Argumento | Descripción |
|---|---|
| `--n N` | Cantidad de elementos del arreglo. |
| `--max M` | Valor máximo generado. |
| `--target X` | Valor fijo que se buscará. |
| `--logs RUTA` | Carpeta donde se guardan los logs. |
| `--detallado` | Muestra el trabajo de cada hilo. |
| `--silencioso` | Desactiva la salida detallada. |

## Ejecución distribuida

Cuando todos los nodos utilizan la misma ruta para el ejecutable:

```bash
cp hosts.example hosts.txt
OMP_NUM_THREADS=2 ./scripts/ejecutar_cluster.sh ./hosts.txt \
  --n 10000000 --max 1000000 --silencioso
```

Para el entorno de tres computadoras con rutas diferentes se incluye
`scripts/ejecutar_tres_nodos_desde_diego.sh`. Antes de usarlo, cada máquina debe
tener su copia compilada en la ruta indicada por el script.

## Logs

Cada proceso crea un archivo con este formato:

```text
log_equipo_NOMBRE_nodo_RANK.txt
```

El modo `--detallado` está pensado para una prueba pequeña y legible. Las
mediciones de rendimiento deben ejecutarse con `--silencioso` y un arreglo de
mayor tamaño.
