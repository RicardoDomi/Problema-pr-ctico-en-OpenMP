# Actividad 1.2 — Arreglos distribuidos con MPI y OpenMP

## Objetivo

Implementar operaciones aritmeticas sobre arreglos dinamicos utilizando dos niveles de paralelismo:

- **MPI** distribuye secciones de los arreglos entre procesos.
- **OpenMP** distribuye entre hilos el procesamiento de cada seccion local.

La demostracion se realiza de manera local con cinco procesos MPI: un proceso maestro y cuatro procesos trabajadores.

## Integrantes

1. Castolo Gonzales Jorge Natanael
2. Dominguez Alcala Ricardo
3. Silva Montes Diego Eduardo

## Implementaciones

### Version 1: `MPI_Send` y `MPI_Recv`

Archivo: `memoria_mpi.cpp`

Esta version se encuentra implementada. El proceso MPI 0 distribuye los datos mediante `MPI_Send`, los procesos trabajadores reciben sus secciones con `MPI_Recv`, procesan los datos mediante OpenMP y envian los resultados parciales al maestro.

### Version 2: `MPI_Scatter` y `MPI_Gather`

Archivo: `memoria_scatter_gather.cpp`

Esta version se encuentra implementada. Utiliza `MPI_Scatter` para distribuir las secciones y `MPI_Gather` para reunirlas. Como ambas operaciones colectivas incluyen al proceso raiz, se reserva un primer bloque auxiliar para MPI 0; los cuatro bloques con datos reales corresponden a los trabajadores MPI 1–4. De esta manera, el maestro participa en las colectivas pero no realiza operaciones aritmeticas.

## Estructura del proyecto

```text
Actividad 1.2/
├── .gitignore
├── Makefile
├── README.md
├── memoria_mpi.cpp                 # Version Send/Recv
└── memoria_scatter_gather.cpp      # Version Scatter/Gather
```

## Arquitectura del programa

```text
Proceso MPI 0 (maestro)
    │
    ├── distribuye una seccion a MPI 1 ── OpenMP procesa con varios hilos
    ├── distribuye una seccion a MPI 2 ── OpenMP procesa con varios hilos
    ├── distribuye una seccion a MPI 3 ── OpenMP procesa con varios hilos
    └── distribuye una seccion a MPI 4 ── OpenMP procesa con varios hilos
    │
    └── recibe y reconstruye el arreglo resultado
```

El maestro no realiza las operaciones aritmeticas. Los cuatro trabajadores reciben cantidades iguales y cada uno utiliza OpenMP para procesar su seccion.

## Clase `OperacionesArreglos`

La clase contiene las siguientes funciones:

- `crearArregloMPI`: crea la seccion local de los arreglos A y B.
- `sumarArreglosOpenMPI`: suma A y B elemento a elemento.
- `restarArreglosOpenMPI`: resta B de A elemento a elemento.
- `multiplicarArreglosOpenMPI`: multiplica A y B elemento a elemento.
- `cuadradoArregloOpenMPI`: calcula el cuadrado de los elementos de A.

Todas trabajan con punteros y procesan solamente la cantidad de elementos asignada al trabajador. Las operaciones internas utilizan `#pragma omp parallel for`.

## Requisitos locales

- Compilador de C++ con soporte para C++17.
- Implementacion de MPI, por ejemplo MPICH u Open MPI.
- OpenMP.

En macOS con Homebrew:

```bash
brew install mpich libomp
```

## Compilacion en macOS

Desde la carpeta `Actividad 1.2`:

```bash
make
```

El `Makefile` detecta macOS y obtiene la ruta de `libomp` mediante Homebrew. El comando equivalente para compilar manualmente la version Send/Recv es:

```bash
mpic++ -std=c++17 \
  -Xpreprocessor -fopenmp \
  -I/opt/homebrew/opt/libomp/include \
  -L/opt/homebrew/opt/libomp/lib \
  -Wl,-rpath,/opt/homebrew/opt/libomp/lib \
  memoria_mpi.cpp -lomp -o memoria_mpi
```

Para compilar manualmente la version colectiva:

```bash
mpic++ -std=c++17 \
  -Xpreprocessor -fopenmp \
  -I/opt/homebrew/opt/libomp/include \
  -L/opt/homebrew/opt/libomp/lib \
  -Wl,-rpath,/opt/homebrew/opt/libomp/lib \
  memoria_scatter_gather.cpp -lomp -o memoria_scatter_gather
```

## Compilacion en Linux

También puede utilizarse `make`. Los comandos manuales equivalentes son:

```bash
mpic++ -std=c++17 -fopenmp memoria_mpi.cpp -o memoria_mpi
mpic++ -std=c++17 -fopenmp memoria_scatter_gather.cpp -o memoria_scatter_gather
```

## Ejecucion local

La actividad requiere cinco procesos: uno maestro y cuatro trabajadores.

```bash
OMP_NUM_THREADS=2 mpirun -np 5 ./memoria_mpi
OMP_NUM_THREADS=2 mpirun -np 5 ./memoria_scatter_gather
```

También están disponibles los siguientes comandos:

```bash
make run-send
make run-scatter
```

El valor de `OMP_NUM_THREADS` puede ajustarse a los nucleos disponibles. Para comprobar claramente la intervencion de varios hilos se recomienda utilizar por lo menos dos.

## Menu

Al iniciar, el programa permite elegir:

1. Demostracion detallada con 40 elementos.
2. Prueba de rendimiento con 4,000,000 de elementos.

Despues presenta las operaciones:

1. Crear arreglos.
2. Sumar arreglos.
3. Restar arreglos.
4. Multiplicar arreglos.
5. Calcular el cuadrado de A.
6. Salir.

Siempre se debe ejecutar primero la opcion **Crear arreglos**.

## Pruebas locales

### Demostracion de 40 elementos

Seleccionar primero el tipo de prueba `1` y despues ejecutar las opciones `1`, `2`, `3`, `4` y `5`.

La salida detallada muestra:

```text
[Equipo: nombre]
[Proceso MPI: numero]
[Hilo OpenMP: numero]
[Posicion: indice]
[Operacion: nombre]
```

Los valores esperados son:

- A contiene los numeros del 1 al 40.
- B contiene los numeros del 40 al 1.
- La suma contiene 41 en todas las posiciones.
- La resta contiene los valores impares desde -39 hasta 39.
- El cuadrado de A comienza con 1, 4, 9, 16 y termina en 1600.

### Prueba de 4,000,000 de elementos

Seleccionar el tipo de prueba `2` y ejecutar nuevamente las cinco operaciones. Los valores de A y B se generan aleatoriamente dentro del rango de 1 a 1,000,000.

En esta prueba se desactivan los mensajes por elemento y solamente se presentan los tiempos, evitando que la salida altere considerablemente la medicion.

## Diferencia entre las dos versiones MPI

| `MPI_Send` / `MPI_Recv` | `MPI_Scatter` / `MPI_Gather` |
|---|---|
| El programador envia y recibe cada seccion explicitamente. | MPI distribuye y reune las secciones mediante operaciones colectivas. |
| Requiere ciclos, destinos, origenes y etiquetas. | Reduce el codigo dedicado a la comunicacion. |
| Permite controlar individualmente cada mensaje. | Resulta apropiado cuando todas las secciones tienen el mismo tamano. |

## Nota sobre el alcance de las pruebas

Las pruebas documentadas para esta entrega se realizaran localmente. El nombre del equipo se conserva en la salida para demostrar la identificacion solicitada de cada proceso MPI, aunque todos se ejecuten en la misma computadora.

## Verificacion realizada

Las dos versiones fueron compiladas y ejecutadas localmente con cinco procesos MPI y dos hilos OpenMP por proceso. En ambas versiones se verificaron las cuatro operaciones con 40 elementos y la ejecucion completa con 4,000,000 de elementos.

En la version Scatter/Gather, la prueba de 40 elementos produjo:

- Suma: 41 en las 40 posiciones.
- Resta: valores impares desde -39 hasta 39.
- Multiplicacion: valores correctos desde `40, 78, 114...` hasta `...114, 78, 40`.
- Cuadrado: cuadrados desde 1 hasta 1600.

### Tiempos de una ejecucion local de referencia

Prueba realizada con cinco procesos MPI y `OMP_NUM_THREADS=2`. Los tiempos pueden cambiar entre ejecuciones y equipos.

| Operacion | Send/Recv | Scatter/Gather |
|---|---:|---:|
| Creacion | 0.070189 s | 0.092048 s |
| Suma | 0.026215 s | 0.041829 s |
| Resta | 0.020486 s | 0.036456 s |
| Multiplicacion | 0.016973 s | 0.029002 s |
| Cuadrado | 0.011124 s | 0.023504 s |

Una sola medicion no permite concluir que un mecanismo sea siempre mas rapido. Para una comparacion formal deben repetirse las pruebas y utilizar promedios.
