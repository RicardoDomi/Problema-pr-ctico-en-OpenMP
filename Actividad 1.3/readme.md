# Actividad 1.3 — MPI y OpenMP con arreglos dinámicos

## Integrantes

1. Castolo Gonzales Jorge Natanael
2. Dominguez Alcala Ricardo
3. Silva Montes Diego Eduardo

## Código incluido

```text
actividad 1.3/
├── readme.md
├── Makefile
├── .gitignore
└── src/
    ├── main.cpp
    ├── OperacionesArreglos.h
    ├── OperacionesArreglos.cpp
    ├── Comunicacion.h
    ├── Comunicacion.cpp
    ├── Registro.h
    └── Registro.cpp
```

`OperacionesArreglos` contiene la creación, el llenado, las operaciones
aritméticas y las reducciones. Los arreglos A, B y C se almacenan mediante
punteros con `new[]` y se liberan con `delete[]`. No se utilizan arreglos
estáticos, `std::vector` ni contenedores equivalentes para los datos.

Las cuatro reducciones son métodos públicos de esta clase: `sumatoriaMPI`,
`promedioMPI`, `maximoMPI` y `minimoMPI`. Reciben el arreglo y un `Contexto`
con nombre del equipo, rank, sección local y tamaño global. Cada método
realiza la reducción local con OpenMP y consolida los parciales mediante
la comunicación de la versión seleccionada. Los pasos auxiliares se
encapsulan como métodos privados. Esta es la organización en clases solicitada
en la actividad; la consigna no requiere herencia ni polimorfismo.

El proceso MPI 0 coordina el menú, transmite las decisiones con `MPI_Bcast` y
reúne los resultados. Cada trabajador procesa únicamente su sección del
arreglo, distribuida entre sus hilos mediante OpenMP.

## Compilación

Se requiere un compilador C++17, MPI, OpenMP y `make`. Desde la carpeta
`actividad 1.3`, en la ubicación elegida por cada integrante:

```bash
make -j3
```

El Makefile utiliza `-fopenmp` en Linux. En macOS utiliza Clang y `libomp`,
buscando esta biblioteca en `/opt/homebrew/opt/libomp` o
`/usr/local/opt/libomp`. Para otra ubicación:

```bash
make LIBOMP_PREFIX=/ruta/a/libomp
```

Se compilan tres versiones con la misma clase y archivos fuente:

| Ejecutable | Comunicación de datos | Reducciones globales |
|---|---|---|
| `arreglos_send_recv` | `MPI_Send` / `MPI_Recv` | Recepción de parciales en el maestro |
| `arreglos_scatter_gather` | `MPI_Scatter` / `MPI_Gather` | Gather de parciales y combinación en el maestro |
| `arreglos_reduce` | `MPI_Scatter` / `MPI_Gather` | `MPI_Reduce` / `MPI_Allreduce` |

## Ejecución local

Cinco procesos MPI: un maestro y cuatro trabajadores. Dos hilos OpenMP por
proceso en estos ejemplos:

```bash
OMP_NUM_THREADS=2 mpirun -np 5 ./arreglos_send_recv --n 40
OMP_NUM_THREADS=2 mpirun -np 5 ./arreglos_scatter_gather --n 40
OMP_NUM_THREADS=2 mpirun -np 5 ./arreglos_reduce --n 40
```

También pueden usarse `make run-send`, `make run-scatter` y `make run-reduce`,
con parámetros `N`, `PROCESSES` y `THREADS`.

## Uso de la carpeta entre integrantes

`actividad 1.3` es una carpeta independiente que contiene los fuentes, el
Makefile y estas instrucciones. Puede copiarse completa a la ubicación que
cada integrante use para sus trabajos por SSH. Los fuentes no fijan usuarios,
IP, rutas personales ni una instalación particular de MPI.

Cada integrante puede revisarla con su Codex y ajustar la compilación a su
entorno. Para una ejecución conjunta conviene acordar la misma revisión final
de los fuentes y una implementación de MPI compatible entre los equipos.
Si se modifica el programa, esos cambios se comparten antes de ejecutar juntos.
Los binarios se compilan en cada computadora para su sistema y arquitectura.

Una forma de organizarse es:

1. Descargar del repositorio o copiar la carpeta completa `actividad 1.3` a
   la ruta accesible por SSH de cada equipo, conservando `src` y el Makefile.
2. Revisar las herramientas disponibles y compilar con el MPI que se utilizará
   en la ejecución conjunta. El compilador puede seleccionarse con
   `make CXX=/ruta/al/mpic++`; las opciones de OpenMP se ajustan según el entorno.
3. Compartir con quien lance la ejecución el host o alias SSH, la ruta local
   de la carpeta y el entorno MPI utilizado.
4. Preparar el lanzamiento desde el maestro con la configuración SSH/MPI
   existente, indicando las rutas de cada equipo cuando sean diferentes.
   MPI inicia los procesos de esa ejecución; rank 0 recibe el menú y coordina.

Los comandos concretos de red se acuerdan al integrar los entornos. El
hostfile, los alias SSH y los posibles lanzadores propios se gestionan en la
configuración de cada integrante. La carpeta puede mantenerse igual en los
tres equipos aunque sus ubicaciones sean distintas.

El programa usa `MPI_Comm_size` para repartir el trabajo según los procesos
lanzados. Con una computadora maestra y dos trabajadoras, una distribución
posible para los cinco procesos de la actividad es uno en el maestro y dos
en cada trabajador. También admite tres procesos, uno por computadora.
La asignación a las máquinas corresponde al lanzador MPI.

En todos los procesos de una misma ejecución se selecciona la misma versión:
`arreglos_send_recv`, `arreglos_scatter_gather` o `arreglos_reduce`. Los
parámetros del arreglo se mantienen iguales entre nodos y cada proceso guarda
sus logs localmente. La ejecución entre las computadoras se comprueba cuando
estén integradas sus rutas y entornos.

## Menú de 12 opciones

```text
1. Crear arreglos
2. Suma
3. Resta
4. Multiplicacion
5. Cuadrado
6. Llenar secuencial
7. Llenar aleatorio
8. Sumatoria
9. Promedio
10. Maximo
11. Minimo
12. Salir
```

Primero seleccionar **1** y después **6** o **7**. Las operaciones 2–5 guardan
su resultado en C. El cuadrado y las reducciones se calculan sobre A.
Crear los arreglos nuevamente libera su memoria anterior y requiere llenarlos
otra vez.

El llenado secuencial genera A[i]=i+1 y B[i]=N−i según la posición global.
El llenado aleatorio genera valores entre 1 y 1,000,000. Cada proceso inicializa
`srand` con una semilla que combina tiempo y rank; cada hilo utiliza un
generador independiente para evitar compartir `rand()` dentro de OpenMP.

En `arreglos_reduce`, al seleccionar las opciones 8–11 se elige **1 para
MPI_Reduce** o **2 para MPI_Allreduce**. Reduce entrega el resultado global al
maestro; Allreduce lo entrega a todos los procesos. La sumatoria local usa
`reduction(+:suma_local)`, el máximo `reduction(max:max_local)` y el mínimo
`reduction(min:min_local)`. El promedio divide la suma global entre N.

## Secciones distribuidas

Cada trabajador recibe un bloque de `ceil(N/(procesos−1))` posiciones.
El maestro participa en Scatter/Gather con un bloque auxiliar sin datos de
trabajo. Si N no se divide exactamente entre los trabajadores, las posiciones
de relleno no se procesan ni se incluyen en las reducciones.

Para 40 elementos y cinco procesos, los trabajadores 1–4 procesan,
respectivamente, las posiciones 0–9, 10–19, 20–29 y 30–39. Los datos se
almacenan como `long long` y se comunican con `MPI_LONG_LONG_INT`, para admitir
los productos y cuadrados de valores de hasta 1,000,000.

MPI se inicializa una sola vez con `MPI_Init_thread` y soporte
`MPI_THREAD_FUNNELED`. Todas las llamadas MPI se realizan desde el hilo
principal, fuera de las regiones paralelas OpenMP.

## Salida detallada, logs y tiempos

Con 40 elementos se muestran los mensajes por elemento en consola y en el
archivo de registro del proceso correspondiente:

```text
[Equipo: PC01] [Proceso MPI: 2] [Hilo OpenMP: 1] [Posicion: 15] [Operacion: Suma] [Valor: 41]
```

Cada proceso crea localmente `logs/log_equipo_NOMBRE_PC_nodo_RANK.txt`.
`Registro` escribe cada línea del programa en consola y en el log con el
mismo texto. Un cerrojo protege las líneas generadas por los hilos. Los
trabajadores guardan sus mensajes inmediatamente en su propio log y el
maestro los presenta por proceso durante la operación detallada, para evitar
que el lanzador mezcle fragmentos de distintas líneas. Esta comunicación
transporta texto de registro; los arreglos utilizan el mecanismo MPI de la
versión seleccionada. Cada log se cierra antes de `MPI_Finalize`. Los nombres del equipo
aparecen como primera y última línea de la ejecución y de cada log.

Para 4,000,000 de elementos:

```bash
OMP_NUM_THREADS=2 mpirun -np 5 ./arreglos_reduce --n 4000000 --logs logs/grande
```

Seleccionar **1**, **7** y después las operaciones. En arreglos grandes se
desactivan las impresiones por elemento. Cada operación se mide con
`MPI_Wtime`, incluyendo comunicación, procesamiento y sincronización final.
Se presenta el tiempo global en el maestro y se guardan tiempos locales y
globales en los logs de todos los procesos. El tiempo global es el máximo de
las duraciones locales, obtenido con MPI_Allreduce y MPI_MAX en las tres
versiones; esta reducción de tiempos es independiente de las de datos.

La ruta de logs puede cambiarse con `--logs DIRECTORIO`. Los archivos se abren
en modo append para conservar los registros anteriores. Los ejecutables y
logs generados se excluyen de Git mediante `.gitignore`.
