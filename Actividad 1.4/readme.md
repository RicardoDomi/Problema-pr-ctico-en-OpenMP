# Actividad 1.4 — Cuenta bancaria Cliente-Servidor con MPI y OpenMP

## Integrantes

1. Castolo Gonzales Jorge Natanael
2. Dominguez Alcala Ricardo
3. Silva Montes Diego Eduardo

## Contenido

```text
actividad 1.4/
├── .gitignore
├── Makefile
├── hosts.example
├── readme.md
├── scripts/
│   ├── ejecutar_cluster.sh
│   └── ejecutar_local.sh
└── src/
    ├── ClienteBancario.cpp
    ├── ClienteBancario.h
    ├── CuentaBancaria.cpp
    ├── CuentaBancaria.h
    ├── Mensajes.cpp
    ├── Mensajes.h
    ├── Registro.cpp
    ├── Registro.h
    ├── ServidorBancario.cpp
    ├── ServidorBancario.h
    └── main.cpp
```

El programa implementa una cuenta bancaria centralizada. El proceso MPI 0 es
el único propietario del saldo y procesa una solicitud a la vez mediante
`MPI_Probe` y `MPI_Recv` con `MPI_ANY_SOURCE`. Los procesos MPI 1 a 4 son
clientes. Cada cliente usa OpenMP para construir sus transacciones en paralelo
y luego el hilo principal las envía ordenadamente al servidor con
`MPI_Send`, esperando cada `RespuestaServidor` con `MPI_Recv`.

La modificación del saldo ocurre solamente en `CuentaBancaria::procesar`,
invocada por el bucle secuencial del servidor. Por eso no se necesitan
candados distribuidos ni memoria compartida entre computadoras.

## Organización orientada a objetos

- `CuentaBancaria` encapsula el saldo y las reglas de depósito, retiro y
  consulta.
- `ServidorBancario` conserva el historial central y atiende solicitudes de
  cualquier cliente hasta recibir `TAG_FIN` de los cuatro clientes.
- `ClienteBancario` genera solicitudes con OpenMP, conserva su historial y se
  comunica con el servidor.
- `TiposMPI` crea los tipos derivados MPI para transmitir `Transaccion` y
  `RespuestaServidor` respetando la distribución real de sus campos.
- `Registro` crea el archivo local de cada nodo y conserva los mensajes.

Los historiales del servidor y de los clientes se almacenan en arreglos
dinámicos administrados mediante punteros, `new[]` y `delete[]`. No se utiliza
`std::vector`, arreglos estáticos ni contenedores equivalentes para los
historiales. El campo `char mensaje[100]` forma parte de la estructura exigida
por la consigna y no se utiliza como historial.

## Mensajes y sincronización

Las estructuras intercambiadas son:

```cpp
struct Transaccion {
    int id_cliente;
    int id_hilo_openmp;
    int tipo_operacion;
    double monto;
};

struct RespuestaServidor {
    bool aprobada;
    double saldo_resultante;
    char mensaje[100];
};
```

Cada cliente genera una mezcla aleatoria de depósitos, retiros y consultas.
La semilla inicial combina la hora y el rank mediante `srand`; a partir de
ella se entrega una semilla independiente a cada hilo OpenMP. Esto evita que
los hilos compartan el estado interno de `rand()`.

Los clientes envían una transacción y esperan su respuesta antes de continuar.
El servidor utiliza `MPI_ANY_SOURCE`, pero modifica el saldo dentro de un solo
bucle secuencial. Los retiros sin fondos se rechazan sin cambiar el saldo.
Al terminar una simulación, cada cliente envía `TAG_FIN`; el servidor sale de
su ciclo al recibir las cuatro señales.

MPI se inicializa con `MPI_THREAD_FUNNELED`: las regiones OpenMP generan los
datos y todas las llamadas MPI se realizan desde el hilo principal.

## Compilación

Se requiere C++17, MPI, OpenMP y `make`:

```bash
make
```

En Linux se utiliza `-fopenmp`. En macOS el Makefile busca `libomp` en
`/opt/homebrew/opt/libomp` o `/usr/local/opt/libomp`. Se puede indicar otra
instalación:

```bash
make CXX=/ruta/a/mpic++ LIBOMP_PREFIX=/ruta/a/libomp
```

En la configuración MPI usada por el integrante que preparó el proyecto:

```bash
"$HOME/mpi-stack/mpi-env" make \
  CXX="$HOME/mpi-stack/openmpi-5.0.10-tailscale/bin/mpic++"
```

## Ejecución local

El ejercicio requiere exactamente cinco procesos: un servidor y cuatro
clientes. Para abrir el menú:

```bash
OMP_NUM_THREADS=2 mpirun --bind-to none -np 5 \
  ./cuenta_bancaria --logs logs/local
```

También puede utilizarse:

```bash
OMP_NUM_THREADS=2 ./scripts/ejecutar_local.sh
```

El menú es:

```text
1. Iniciar Simulacion Basica (5 operaciones por cliente)
2. Iniciar Simulacion Masiva (100000 operaciones por cliente)
3. Consultar Saldo Final del Servidor
4. Salir
```

La opción 1 procesa 20 transacciones y muestra cada respuesta con computadora,
rank cliente, hilo OpenMP, operación, monto, estado y saldo resultante. La
opción 2 procesa 400,000 transacciones; no las imprime una por una en pantalla,
pero conserva el historial en los logs. Al finalizar muestra solamente tiempo
global, transacciones procesadas y saldo consolidado. El saldo inicia en
`$10,000.00` y se conserva entre opciones mientras el programa siga abierto.

## Logs persistentes

Cada proceso crea en su propia computadora:

```text
logs/.../log_equipo_NOMBRE_PC_nodo_RANK.txt
```

El encabezado registra computadora, rank y número de hilos. Los clientes
guardan cada solicitud y su respuesta; el servidor guarda cada solicitud
procesada y cada señal de finalización. Los mensajes detallados de los clientes
se presentan en pantalla por orden de rank después de la simulación básica para
evitar líneas fragmentadas por el lanzador MPI. Los tiempos se obtienen con
`MPI_Wtime()` y quedan registrados en todos los nodos.

Los nombres completos de los integrantes aparecen como primera y última línea
de la salida normal y de cada archivo de log.

## Ejecución distribuida

Todos los integrantes deben copiar esta misma carpeta y compilarla localmente
con una implementación compatible de MPI. No se incluyen usuarios, claves,
direcciones IP ni rutas SSH personales. Copiar `hosts.example` como
`hosts.txt` y sustituir los nombres por los hosts o alias SSH reales:

```text
maestro slots=1
cliente1 slots=2
cliente2 slots=2
```

El orden reserva el primer rank para el servidor y distribuye dos clientes en
cada una de las otras computadoras. Para una instalación Open MPI:

```bash
OMP_NUM_THREADS=2 ./scripts/ejecutar_cluster.sh hosts.txt
```

El ejecutable debe existir en todos los equipos y ser accesible mediante la
misma ruta usada por `mpirun`, o el lanzador del equipo debe resolver las rutas
propias. La configuración SSH, el entorno MPI y las rutas particulares se
integran después en `mpi-stack`; el código de la actividad permanece común a
los tres integrantes.

## Correspondencia con la consigna

| Requisito | Implementación |
|---|---|
| Servidor único, nodo 0 | `ServidorBancario` y `CuentaBancaria` |
| Clientes, nodos 1 a 4 | `ClienteBancario` |
| Recepción desde cualquier cliente | `MPI_ANY_SOURCE` |
| Depósito, retiro y consulta | `CuentaBancaria::procesar` |
| Respuesta aprobada o rechazada | `RespuestaServidor` |
| OpenMP en cada cliente | Región `omp parallel` y `omp for` |
| Historiales dinámicos | Punteros con `new[]` y `delete[]` |
| Sin `std::vector` | No se usa para transacciones ni respuestas |
| 5 operaciones por cliente | Opción 1 |
| 100,000 operaciones por cliente | Opción 2 |
| Logs locales por nodo | Clase `Registro` |
| Tiempo total | `MPI_Wtime()` y máximo global |
| Primera y última línea con nombres | `main.cpp` y logs |
| Scripts local y distribuido | Carpeta `scripts/` |

Los entregables externos de la actividad —video público y documento PDF con
diagrama, explicación, tiempos y evidencias— se elaboran después de realizar
las ejecuciones local y distribuida.
