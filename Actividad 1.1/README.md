# Actividad 1.1: programación híbrida con MPI y OpenMP

Este programa utiliza MPI para distribuir la ejecución entre procesos y nodos, y OpenMP para llenar en paralelo un arreglo dinámico dentro de cada proceso. El almacenamiento se reserva con `new[]` y se libera con `delete[]`; no se utiliza `std::vector` ni una estructura equivalente.

El mismo código se utiliza en las tres pruebas. Solamente cambian el tamaño del arreglo, la cantidad de procesos y los equipos incluidos en la ejecución.

## Requisitos

- Compilador compatible con C++17.
- Una implementación de MPI instalada en todos los nodos.
- OpenMP disponible en todos los nodos.
- Para la prueba distribuida: Tailscale activo, acceso SSH sin contraseña y la misma implementación de MPI en las tres computadoras.

## Compilación

En Linux con GCC:

```sh
mpic++ -std=c++17 -O2 -Wall -Wextra -fopenmp modelo_de_programación.cpp -o modelo
```

En macOS con Apple Clang y `libomp` de Homebrew:

```sh
mpic++ -std=c++17 -O2 -Wall -Wextra \
  -Xpreprocessor -fopenmp \
  -I/opt/homebrew/opt/libomp/include \
  modelo_de_programación.cpp \
  -L/opt/homebrew/opt/libomp/lib \
  -Wl,-rpath,/opt/homebrew/opt/libomp/lib -lomp \
  -o modelo
```

Si se usa el stack de Open MPI instalado en `~/mpi-stack`, anteponer `~/mpi-stack/mpi-env` al comando de compilación y a `mpirun`:

```sh
~/mpi-stack/mpi-env mpic++ -std=c++17 -O2 \
  -Xpreprocessor -fopenmp \
  -I/opt/homebrew/opt/libomp/include \
  modelo_de_programación.cpp \
  -L/opt/homebrew/opt/libomp/lib \
  -Wl,-rpath,/opt/homebrew/opt/libomp/lib -lomp \
  -o modelo
```

## Parámetros

```text
--pequeno              Tamaño aleatorio entre 20 y 50 (valor predeterminado).
--n N                  Tamaño solicitado; para las pruebas grandes usar 10000000.
--equipo NOMBRE        Nombre del equipo de estudiantes.
--salida CARPETA       Carpeta donde se escriben los arreglos por proceso.
--solo-archivos        No vuelve a copiar el contenido completo a la terminal.
```

La carpeta indicada con `--salida` debe existir antes de ejecutar el programa.

## Primera prueba: una computadora y 20–50 elementos

En una computadora de 8 núcleos, cuatro procesos con dos hilos cada uno utilizan los ocho núcleos sin sobresuscribirlos:

```sh
mkdir -p resultados/pequena
OMP_NUM_THREADS=2 mpirun -np 4 ./modelo \
  --pequeno --equipo "NOMBRE-DEL-EQUIPO" \
  --salida resultados/pequena
```

Cambiar `OMP_NUM_THREADS` de acuerdo con los núcleos disponibles y la cantidad de procesos MPI.

## Segunda prueba: una computadora y 10,000,000 elementos

```sh
mkdir -p resultados/grande
OMP_NUM_THREADS=2 mpirun -np 4 ./modelo \
  --n 10000000 --equipo "NOMBRE-DEL-EQUIPO" \
  --salida resultados/grande \
  > resultados/grande/ejecucion.log 2>&1
```

El archivo `ejecucion.log` conserva la salida completa. Para revisar los porcentajes sin mostrar millones de valores:

```sh
grep '^AVANCE' resultados/grande/ejecucion.log
grep -E '^(RESULTADO|FINAL)' resultados/grande/ejecucion.log
```

Durante ensayos rápidos puede agregarse `--solo-archivos`. Los arreglos completos seguirán disponibles en `arreglo-proceso-0.txt`, `arreglo-proceso-1.txt`, etc.

## Tercera prueba: maestro y dos esclavos

Copiar `hosts.example` como `hosts` y sustituir los alias por los nombres SSH o direcciones Tailscale reales. Se recomienda un proceso MPI por nodo para que OpenMP utilice los núcleos internos de cada computadora.

El ejecutable debe estar disponible en la misma ruta de trabajo en los tres nodos. Si no existe un sistema de archivos compartido, hay que copiarlo a cada computadora antes de iniciar la prueba.

Desde el nodo maestro:

```sh
mpirun -np 3 --hostfile hosts \
  --map-by ppr:1:node --bind-to none \
  ./modelo --n 10000000 --equipo "NOMBRE-DEL-EQUIPO" \
  > ejecucion-distribuida.log 2>&1
```

El resultado debe contener tres nombres de nodo distintos y diez avances por proceso:

```sh
grep '^INICIO' ejecucion-distribuida.log
grep '^AVANCE' ejecucion-distribuida.log
grep -E '^(RESULTADO|FINAL)' ejecucion-distribuida.log
```

Cada mensaje diferencia explícitamente:

- `proceso`: rango MPI y total de procesos;
- `nodo`: nombre devuelto por `MPI_Get_processor_name`;
- `equipo`: nombre del equipo indicado con `--equipo`;
- `hilo`: identificador del hilo OpenMP que imprimió el avance.

## Archivos generados

Cada proceso crea un archivo independiente llamado `arreglo-proceso-RANGO.txt`. El archivo contiene la identificación del proceso, nodo y equipo, seguida por todos los valores del arreglo dinámico.

La opción `--solo-archivos` evita volver a imprimir esos valores en la salida estándar, pero no omite su generación ni almacenamiento.
