# Práctica 1.6: Fuerza bruta distribuida con MPI y OpenMP

Aplicación híbrida maestro-esclavo para localizar una clave formada por `A-Z` y
`0-9`. El espacio se representa numéricamente en base 36 y no se almacena por
completo en memoria.

## Integrantes

- Castolo Gonzales Jorge Natanael
- Dominguez Alcala Ricardo
- Silva Montes Diego Eduardo

## Arquitectura

- El proceso MPI `0` valida la clave, calcula `36^N`, distribuye rangos y espera
  resultados mediante `MPI_Iprobe`.
- Los procesos `1..N-1` reciben su rango con `MPI_Recv` y lo procesan con
  OpenMP usando `schedule(dynamic)`.
- El hilo ganador notifica inmediatamente al maestro con `MPI_Ssend`.
- El maestro envía una parada no bloqueante a todos los trabajadores.
- Cada proceso conserva su propio archivo
  `log_equipo_[NOMBRE_PC]_nodo_[RANK].txt`.
- Los rangos, buffers, nombres de equipo y solicitudes MPI utilizan memoria
  dinámica. No se utiliza `std::vector` ni arreglos globales.

El modo de tres caracteres imprime cada candidato. Para claves mayores se
desactiva esa salida y se registran avances periódicos.

## Archivos

```text
actividad 1.6/
├── src/
│   ├── main.cpp
│   ├── FuerzaBruta.h
│   ├── FuerzaBruta.cpp
│   ├── Registro.h
│   └── Registro.cpp
├── scripts/
│   ├── ejecutar_local.sh
│   └── ejecutar_cluster.sh
├── hosts.example
├── Makefile
└── README.md
```

## Compilación

Requiere MPI, un compilador C++17 y OpenMP. En macOS también requiere `libomp`.

```bash
make
```

## Prueba local obligatoria

Ejecuta cinco procesos MPI: un maestro y cuatro trabajadores.

```bash
OMP_NUM_THREADS=2 make run
```

O mediante el script:

```bash
OMP_NUM_THREADS=2 ./scripts/ejecutar_local.sh
```

Secuencia recomendada para una clave de tres caracteres:

1. Opción `1` e ingresar `A0B`.
2. Opción `2` para la línea base secuencial.
3. Opción `3` para MPI + OpenMP.
4. Opción `4` para Speedup y Eficiencia.
5. Opción `5` para finalizar limpiamente.

## Ejecución distribuida

1. Copiar el proyecto y compilarlo en las tres computadoras.
2. Copiar `hosts.example` como `hosts.txt` y colocar los nombres o IP reales.
3. Confirmar acceso SSH sin interacción y la misma ruta del ejecutable en los
   nodos, o definir `APP` y `LOG_DIR` para el entorno compartido.
4. Ejecutar:

```bash
OMP_NUM_THREADS=2 ./scripts/ejecutar_cluster.sh ./hosts.txt
```

El archivo de ejemplo asigna cinco procesos: uno al maestro y dos a cada equipo
esclavo. Para equipos con rutas distintas se puede utilizar la sintaxis de
múltiples aplicaciones de `mpirun`; esa configuración se prepara al migrar la
actividad al entorno `mpi-stack`.

## Prueba de escalabilidad

- `36^3 = 46,656`: salida detallada y demostración completa.
- `36^7 = 78,364,164,096`: prueba masiva autorizada si diez caracteres exceden
  diez minutos.
- `36^10 = 3,656,158,440,062,976`: prueba solicitada para demostrar el costo
  exponencial.

El programa acepta claves de hasta 12 caracteres porque `36^12` cabe en un
`unsigned long long`. Para el reporte se deben documentar los tiempos obtenidos,
el Speedup `T1/Tp` y la Eficiencia `Speedup/p`.

## Evidencias esperadas

- Inicio y finalización de los cinco procesos.
- Rangos asignados a los cuatro trabajadores.
- Equipo, rank e hilo que localizaron la clave.
- Orden global de parada temprana.
- Tabla secuencial frente a MPI + OpenMP.
- Logs independientes de las tres computadoras.
- IP, hostfile y comando usado para la ejecución remota.
