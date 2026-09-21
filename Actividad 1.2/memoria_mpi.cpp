/*
Castolo Gonzales Jorge Natanael
Dominguez Alcala Ricardo
Silva Montes Diego Eduardo

Actividad 1.2 - Memoria distribuida con MPI y OpenMP

Arquitectura de esta version:
1. El proceso MPI 0 funciona exclusivamente como maestro.
2. Los procesos MPI 1..N funcionan como trabajadores.
3. El maestro distribuye las secciones mediante MPI_Send.
4. Cada trabajador procesa su seccion mediante hilos OpenMP.
5. Los trabajadores devuelven los resultados mediante MPI_Send y el
   maestro los reconstruye mediante MPI_Recv.

Este archivo corresponde a la primera implementacion solicitada:
comunicacion punto a punto con MPI_Send y MPI_Recv.
*/

#include <mpi.h>
#include <omp.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <random>

class OperacionesArreglos
{
public:
    // Suma dos secciones locales. Cada iteracion del ciclo puede ser
    // atendida por un hilo OpenMP diferente.
    void sumarArreglosOpenMPI(
        long long* A,
        long long* B,
        long long* R,
        int N,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        // OpenMP reparte los indices de esta seccion entre los hilos.
        #pragma omp parallel for
        for (int i = 0; i < N; i++)
        {
            R[i] = A[i] + B[i];

            if (detallado)
            {
                int posicionGlobal = (mpi_rank - 1) * N + i;

                // Evita que las lineas de varios hilos se mezclen en pantalla.
                #pragma omp critical
                {
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] [Hilo OpenMP: %d] "
                        "[Posicion: %d] [Operacion: Suma]\n",
                        hostname,
                        mpi_rank,
                        omp_get_thread_num(),
                        posicionGlobal
                    );
                }
            }
        }
    }

    // Resta B de A elemento a elemento sobre la seccion local recibida.
    void restarArreglosOpenMPI(
        long long* A,
        long long* B,
        long long* R,
        int N,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < N; i++)
        {
            R[i] = A[i] - B[i];

            if (detallado)
            {
                int posicionGlobal = (mpi_rank - 1) * N + i;

                #pragma omp critical
                {
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] [Hilo OpenMP: %d] "
                        "[Posicion: %d] [Operacion: Resta]\n",
                        hostname,
                        mpi_rank,
                        omp_get_thread_num(),
                        posicionGlobal
                    );
                }
            }
        }
    }

    // Multiplica A por B elemento a elemento sobre la seccion local.
    void multiplicarArreglosOpenMPI(
        long long* A,
        long long* B,
        long long* R,
        int N,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < N; i++)
        {
            R[i] = A[i] * B[i];

            if (detallado)
            {
                int posicionGlobal = (mpi_rank - 1) * N + i;

                #pragma omp critical
                {
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] [Hilo OpenMP: %d] "
                        "[Posicion: %d] [Operacion: Multiplicacion]\n",
                        hostname,
                        mpi_rank,
                        omp_get_thread_num(),
                        posicionGlobal
                    );
                }
            }
        }
    }

    // Calcula el cuadrado de cada elemento de la seccion local de A.
    void cuadradoArregloOpenMPI(
        long long* A,
        long long* R,
        int N,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < N; i++)
        {
            R[i] = A[i] * A[i];

            if (detallado)
            {
                int posicionGlobal = (mpi_rank - 1) * N + i;

                #pragma omp critical
                {
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] [Hilo OpenMP: %d] "
                        "[Posicion: %d] [Operacion: Cuadrado]\n",
                        hostname,
                        mpi_rank,
                        omp_get_thread_num(),
                        posicionGlobal
                    );
                }
            }
        }
    }

// Crea unicamente la seccion que pertenece al proceso trabajador actual.
// En la demostracion usa valores consecutivos faciles de verificar; en la
// prueba grande usa valores pseudoaleatorios entre 1 y 1,000,000.
void crearArregloMPI(
    long long* A_local,
    long long* B_local,
    int cantidad,
    int N_total,
    const char* hostname,
    int mpi_rank,
    bool detallado
)
{
    #pragma omp parallel
    {
        int hilo = omp_get_thread_num();

        // Generador independiente por hilo
        std::mt19937 generador(
            static_cast<unsigned int>(
                time(NULL)
                + mpi_rank * 10000
                + hilo * 100
            )
        );

        std::uniform_int_distribution<int> distribucion(1, 1000000);

        #pragma omp for
        for (int i = 0; i < cantidad; i++)
        {
            // Convierte el indice local del trabajador a su posicion dentro
            // del arreglo global que reconstruira el proceso maestro.
            int posicionGlobal =
                (mpi_rank - 1) * cantidad + i;

            // Primera prueba: valores fáciles de comprobar
            if (N_total == 40)
            {
                A_local[i] = posicionGlobal + 1;
                B_local[i] = N_total - posicionGlobal;
            }

            // Segunda prueba: valores aleatorios
            else
            {
                A_local[i] = distribucion(generador);
                B_local[i] = distribucion(generador);
            }

            // Solo imprimir en la prueba de 40
            if (detallado)
            {
                #pragma omp critical
                {
                    printf(
                        "[Equipo: %s] "
                        "[Proceso MPI: %d] "
                        "[Hilo OpenMP: %d] "
                        "[Posicion: %d] "
                        "[A: %lld] "
                        "[B: %lld]\n",
                        hostname,
                        mpi_rank,
                        hilo,
                        posicionGlobal,
                        A_local[i],
                        B_local[i]
                    );
                }
            }
        }
    }
}
};

void mostrarArreglo(long long* arreglo, int N)
{
    for (int i = 0; i < N; i++)
    {
        printf("%lld ", arreglo[i]);
    }
    printf("\n");
}

int main(int argc, char* argv[])
{
    int mpi_rank, mpi_size;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int hostname_len;

    // Inicializacion del entorno MPI e identificacion de cada proceso.
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Get_processor_name(hostname, &hostname_len);

    // La arquitectura maestro-trabajador requiere por lo menos dos procesos.
    if (mpi_size < 2)
    {
        if (mpi_rank == 0)
        {
            printf("Error: se necesita al menos 1 proceso maestro y 1 trabajador.\n");
        }

        MPI_Finalize();
        return 1;
    }

    if (mpi_rank == 0)
    {
        printf("Castolo Gonzales Jorge Natanael\n"
               "Dominguez Alcala Ricardo\n"
               "Silva Montes Diego Eduardo\n\n");
    }

    OperacionesArreglos operaciones;

    // El proceso 0 es el maestro; todos los procesos restantes trabajan.
    int trabajadores = mpi_size - 1;
    int opcion = 0;
    int tipoPrueba = 0;
    int N = 0;
    bool arreglosCreados = false;

    double inicio = 0.0;
    double fin = 0.0;

    // Las etiquetas distinguen los arreglos y cada tipo de resultado en los
    // mensajes punto a punto MPI_Send/MPI_Recv.
    const int TAG_CREAR_A = 0;
    const int TAG_CREAR_B = 1;
    const int TAG_SUMA = 2;
    const int TAG_RESTA = 3;
    const int TAG_MULTIPLICACION = 4;
    const int TAG_CUADRADO = 5;
    const int TAG_A = 10;
    const int TAG_B = 11;

    long long* A = nullptr;
    long long* B = nullptr;
    long long* R = nullptr;

    if (mpi_rank == 0)
    {
        do
        {
            printf("============================================\n");
            printf(" MODELO DE MEMORIA DISTRIBUIDA MPI + OPENMP\n");
            printf("============================================\n");
            printf("Seleccione el tipo de ejecucion:\n");
            printf("1. Demostracion con 40 elementos\n");
            printf("2. Prueba con 4,000,000 elementos\n");
            printf("Opcion: ");
            fflush(stdout);
            scanf("%d", &tipoPrueba);

            if (tipoPrueba == 1)
            {
                N = 40;
            }
            else if (tipoPrueba == 2)
            {
                N = 4000000;
            }
            else
            {
                printf("\nOpcion invalida. Intente nuevamente.\n\n");
            }
        }
        while (tipoPrueba != 1 && tipoPrueba != 2);
    }

    // Todos los procesos deben conocer el tamano seleccionado por el maestro.
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (N % trabajadores != 0)
    {
        if (mpi_rank == 0)
        {
            printf(
                "Error: %d elementos no pueden dividirse equitativamente "
                "entre %d trabajadores.\n",
                N,
                trabajadores
            );
        }

        MPI_Finalize();
        return 1;
    }

    // Cada trabajador recibe exactamente la misma cantidad de elementos.
    int cantidad = N / trabajadores;

    long long* A_local = nullptr;
    long long* B_local = nullptr;
    long long* R_local = nullptr;

    // Cada trabajador almacena solamente su seccion local.
    if (mpi_rank != 0)
    {
        A_local = new long long[cantidad];
        B_local = new long long[cantidad];
        R_local = new long long[cantidad];
    }

    // El maestro conserva los arreglos completos para distribuir y reunir.
    if (mpi_rank == 0)
    {
        A = new long long[N];
        B = new long long[N];
        R = new long long[N];
    }

    if (N == 40)
    {
        printf(
            "[Equipo: %s] [Proceso MPI: %d de %d] [%s]\n",
            hostname,
            mpi_rank,
            mpi_size,
            mpi_rank == 0 ? "MAESTRO" : "TRABAJADOR"
        );
    }

    do
    {
        if (mpi_rank == 0)
        {
            printf("\n============================================\n");
            printf(" MENU\n");
            printf("============================================\n");
            printf("1. Crear arreglos\n");
            printf("2. Sumar arreglos\n");
            printf("3. Restar arreglos\n");
            printf("4. Multiplicar arreglos\n");
            printf("5. Cuadrado de un arreglo\n");
            printf("6. Salir del programa\n");
            printf("Seleccione una opcion: ");
            fflush(stdout);
            scanf("%d", &opcion);
            printf("\n");
        }

        // Solo el maestro lee el teclado y comunica la seleccion al resto.
        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

        switch (opcion)
        {
        case 1:
        {
            // CREACION: los trabajadores generan en paralelo sus secciones y
            // el maestro las recibe para reconstruir A y B completos.
            MPI_Barrier(MPI_COMM_WORLD);

            if (mpi_rank == 0)
            {
                inicio = MPI_Wtime();
            }

            if (mpi_rank != 0)
            {
                operaciones.crearArregloMPI(
                    A_local,
                    B_local,
                    cantidad,
                    N,
                    hostname,
                    mpi_rank,
                    N == 40
                );

                if (N == 40)
                {
                    int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[Enviando seccion: %d - %d] [Destino MPI: 0] "
                        "[Arreglos: A y B]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1
                    );
                }

                MPI_Send(
                    A_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_CREAR_A,
                    MPI_COMM_WORLD
                );

                MPI_Send(
                    B_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_CREAR_B,
                    MPI_COMM_WORLD
                );
            }
            else
            {
                for (int origen = 1; origen < mpi_size; origen++)
                {
                    int inicioSeccion = (origen - 1) * cantidad;

                    MPI_Recv(
                        &A[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        origen,
                        TAG_CREAR_A,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE
                    );

                    MPI_Recv(
                        &B[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        origen,
                        TAG_CREAR_B,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE
                    );

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Seccion recibida: %d - %d] [Origen MPI: %d] "
                            "[Arreglos: A y B]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            origen
                        );
                    }
                }

                fin = MPI_Wtime();

                if (N == 40)
                {
                    printf("\nArreglo A:\n");
                    mostrarArreglo(A, N);

                    printf("\nArreglo B:\n");
                    mostrarArreglo(B, N);

                    printf("\nArreglos creados correctamente.\n");
                }

                printf("Tiempo de creacion: %.10f segundos\n", fin - inicio);
            }

            arreglosCreados = true;
            break;
        }

        case 2:
        {
            // SUMA: el maestro envia A y B por secciones, cada trabajador suma
            // con OpenMP y el maestro recibe cada resultado parcial.
            if (!arreglosCreados)
            {
                if (mpi_rank == 0)
                {
                    printf("Primero debe crear los arreglos con la opcion 1.\n");
                }
                break;
            }

            MPI_Barrier(MPI_COMM_WORLD);

            if (mpi_rank == 0)
            {
                inicio = MPI_Wtime();

                for (int destino = 1; destino < mpi_size; destino++)
                {
                    int inicioSeccion = (destino - 1) * cantidad;

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Enviando seccion: %d - %d] [Destino MPI: %d] "
                            "[Operacion: Suma]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            destino
                        );
                    }

                    MPI_Send(
                        &A[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_A,
                        MPI_COMM_WORLD
                    );

                    MPI_Send(
                        &B[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_B,
                        MPI_COMM_WORLD
                    );
                }

                for (int origen = 1; origen < mpi_size; origen++)
                {
                    int inicioSeccion = (origen - 1) * cantidad;

                    MPI_Recv(
                        &R[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        origen,
                        TAG_SUMA,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE
                    );

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Resultado recibido: %d - %d] [Origen MPI: %d] "
                            "[Operacion: Suma]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            origen
                        );
                    }
                }

                fin = MPI_Wtime();

                if (N == 40)
                {
                    printf("\nResultado suma:\n");
                    mostrarArreglo(R, N);
                    printf("\nOperacion: SUMA\n");
                }

                printf("Tiempo suma: %.10f segundos\n", fin - inicio);
            }
            else
            {
                MPI_Recv(
                    A_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_A,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                MPI_Recv(
                    B_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_B,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                if (N == 40)
                {
                    int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[Seccion recibida: %d - %d] [Operacion: Suma]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1
                    );
                }

                operaciones.sumarArreglosOpenMPI(
                    A_local,
                    B_local,
                    R_local,
                    cantidad,
                    hostname,
                    mpi_rank,
                    N == 40
                );

                MPI_Send(
                    R_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_SUMA,
                    MPI_COMM_WORLD
                );
            }

            break;
        }

        case 3:
        {
            // RESTA: conserva el mismo esquema maestro-trabajador de la suma.
            if (!arreglosCreados)
            {
                if (mpi_rank == 0)
                {
                    printf("Primero debe crear los arreglos con la opcion 1.\n");
                }
                break;
            }

            MPI_Barrier(MPI_COMM_WORLD);

            if (mpi_rank == 0)
            {
                inicio = MPI_Wtime();

                for (int destino = 1; destino < mpi_size; destino++)
                {
                    int inicioSeccion = (destino - 1) * cantidad;

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Enviando seccion: %d - %d] [Destino MPI: %d] "
                            "[Operacion: Resta]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            destino
                        );
                    }

                    MPI_Send(
                        &A[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_A,
                        MPI_COMM_WORLD
                    );

                    MPI_Send(
                        &B[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_B,
                        MPI_COMM_WORLD
                    );
                }

                for (int origen = 1; origen < mpi_size; origen++)
                {
                    int inicioSeccion = (origen - 1) * cantidad;

                    MPI_Recv(
                        &R[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        origen,
                        TAG_RESTA,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE
                    );

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Resultado recibido: %d - %d] [Origen MPI: %d] "
                            "[Operacion: Resta]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            origen
                        );
                    }
                }

                fin = MPI_Wtime();

                if (N == 40)
                {
                    printf("\nResultado resta:\n");
                    mostrarArreglo(R, N);
                    printf("\nOperacion: RESTA\n");
                }

                printf("Tiempo resta: %.10f segundos\n", fin - inicio);
            }
            else
            {
                MPI_Recv(
                    A_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_A,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                MPI_Recv(
                    B_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_B,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                if (N == 40)
                {
                    int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[Seccion recibida: %d - %d] [Operacion: Resta]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1
                    );
                }

                operaciones.restarArreglosOpenMPI(
                    A_local,
                    B_local,
                    R_local,
                    cantidad,
                    hostname,
                    mpi_rank,
                    N == 40
                );

                MPI_Send(
                    R_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_RESTA,
                    MPI_COMM_WORLD
                );
            }

            break;
        }

        case 4:
        {
            // MULTIPLICACION: operacion elemento a elemento sobre cada seccion.
            if (!arreglosCreados)
            {
                if (mpi_rank == 0)
                {
                    printf("Primero debe crear los arreglos con la opcion 1.\n");
                }
                break;
            }

            MPI_Barrier(MPI_COMM_WORLD);

            if (mpi_rank == 0)
            {
                inicio = MPI_Wtime();

                for (int destino = 1; destino < mpi_size; destino++)
                {
                    int inicioSeccion = (destino - 1) * cantidad;

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Enviando seccion: %d - %d] [Destino MPI: %d] "
                            "[Operacion: Multiplicacion]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            destino
                        );
                    }

                    MPI_Send(
                        &A[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_A,
                        MPI_COMM_WORLD
                    );

                    MPI_Send(
                        &B[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_B,
                        MPI_COMM_WORLD
                    );
                }

                for (int origen = 1; origen < mpi_size; origen++)
                {
                    int inicioSeccion = (origen - 1) * cantidad;

                    MPI_Recv(
                        &R[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        origen,
                        TAG_MULTIPLICACION,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE
                    );

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Resultado recibido: %d - %d] [Origen MPI: %d] "
                            "[Operacion: Multiplicacion]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            origen
                        );
                    }
                }

                fin = MPI_Wtime();

                if (N == 40)
                {
                    printf("\nResultado multiplicacion:\n");
                    mostrarArreglo(R, N);
                    printf("\nOperacion: MULTIPLICACION\n");
                }

                printf("Tiempo multiplicacion: %.10f segundos\n", fin - inicio);
            }
            else
            {
                MPI_Recv(
                    A_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_A,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                MPI_Recv(
                    B_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_B,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                if (N == 40)
                {
                    int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[Seccion recibida: %d - %d] "
                        "[Operacion: Multiplicacion]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1
                    );
                }

                operaciones.multiplicarArreglosOpenMPI(
                    A_local,
                    B_local,
                    R_local,
                    cantidad,
                    hostname,
                    mpi_rank,
                    N == 40
                );

                MPI_Send(
                    R_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_MULTIPLICACION,
                    MPI_COMM_WORLD
                );
            }

            break;
        }

        case 5:
        {
            // CUADRADO: solo es necesario distribuir la seccion del arreglo A.
            if (!arreglosCreados)
            {
                if (mpi_rank == 0)
                {
                    printf("Primero debe crear los arreglos con la opcion 1.\n");
                }
                break;
            }

            MPI_Barrier(MPI_COMM_WORLD);

            if (mpi_rank == 0)
            {
                inicio = MPI_Wtime();

                for (int destino = 1; destino < mpi_size; destino++)
                {
                    int inicioSeccion = (destino - 1) * cantidad;

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Enviando seccion: %d - %d] [Destino MPI: %d] "
                            "[Operacion: Cuadrado]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            destino
                        );
                    }

                    MPI_Send(
                        &A[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        destino,
                        TAG_A,
                        MPI_COMM_WORLD
                    );
                }

                for (int origen = 1; origen < mpi_size; origen++)
                {
                    int inicioSeccion = (origen - 1) * cantidad;

                    MPI_Recv(
                        &R[inicioSeccion],
                        cantidad,
                        MPI_LONG_LONG,
                        origen,
                        TAG_CUADRADO,
                        MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE
                    );

                    if (N == 40)
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[Resultado recibido: %d - %d] [Origen MPI: %d] "
                            "[Operacion: Cuadrado]\n",
                            hostname,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            origen
                        );
                    }
                }

                fin = MPI_Wtime();

                if (N == 40)
                {
                    printf("\nResultado cuadrado de A:\n");
                    mostrarArreglo(R, N);
                    printf("\nOperacion: CUADRADO\n");
                }

                printf("Tiempo cuadrado: %.10f segundos\n", fin - inicio);
            }
            else
            {
                MPI_Recv(
                    A_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_A,
                    MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE
                );

                if (N == 40)
                {
                    int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[Seccion recibida: %d - %d] [Operacion: Cuadrado]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1
                    );
                }

                operaciones.cuadradoArregloOpenMPI(
                    A_local,
                    R_local,
                    cantidad,
                    hostname,
                    mpi_rank,
                    N == 40
                );

                MPI_Send(
                    R_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    TAG_CUADRADO,
                    MPI_COMM_WORLD
                );
            }

            break;
        }

        case 6:
        {
            if (mpi_rank == 0)
            {
                printf("Saliendo del programa...\n");
            }
            break;
        }

        default:
        {
            if (mpi_rank == 0)
            {
                printf("Opcion invalida. Seleccione una opcion del 1 al 6.\n");
            }
            break;
        }
        }
    }
    while (opcion != 6);

    // Cada proceso libera exclusivamente la memoria que reservo.
    if (mpi_rank == 0)
    {
        delete[] A;
        delete[] B;
        delete[] R;
    }
    else
    {
        delete[] A_local;
        delete[] B_local;
        delete[] R_local;
    }

    // La actividad solicita mostrar nuevamente a los integrantes al finalizar.
    if (mpi_rank == 0)
    {
        printf("\nCastolo Gonzales Jorge Natanael\n"
               "Dominguez Alcala Ricardo\n"
               "Silva Montes Diego Eduardo\n");
    }

    // En este punto todas las regiones OpenMP ya terminaron; MPI puede cerrar.
    MPI_Finalize();
    return 0;
}
