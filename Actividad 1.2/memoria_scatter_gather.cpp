/*
Castolo Gonzales Jorge Natanael
Dominguez Alcala Ricardo
Silva Montes Diego Eduardo

Actividad 1.2 - Version MPI_Scatter y MPI_Gather

Arquitectura:
1. El proceso MPI 0 es el maestro y no realiza operaciones aritmeticas.
2. Los procesos MPI 1..N son trabajadores.
3. MPI_Scatter distribuye una seccion a cada proceso.
4. Cada trabajador procesa su seccion mediante OpenMP.
5. MPI_Gather reune los resultados parciales en el maestro.

MPI_Scatter y MPI_Gather incluyen al proceso raiz. Para conservar el modelo
de un maestro y cuatro trabajadores, los buffers colectivos reservan un
primer bloque auxiliar para MPI 0. Los bloques reales comienzan en MPI 1.
*/

#include <mpi.h>
#include <omp.h>
#include <cstdio>
#include <ctime>
#include <random>

class OperacionesArreglos
{
public:
    // Cada trabajador crea solamente la seccion que tiene asignada.
    void crearArregloMPI(
        long long* A_local,
        long long* B_local,
        int cantidad,
        int N_total,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel
        {
            const int hilo = omp_get_thread_num();
            std::mt19937 generador(
                static_cast<unsigned int>(
                    time(nullptr) + mpi_rank * 10000 + hilo * 100
                )
            );
            std::uniform_int_distribution<int> distribucion(1, 1000000);

            #pragma omp for
            for (int i = 0; i < cantidad; i++)
            {
                const int posicionGlobal = (mpi_rank - 1) * cantidad + i;

                if (N_total == 40)
                {
                    A_local[i] = posicionGlobal + 1;
                    B_local[i] = N_total - posicionGlobal;
                }
                else
                {
                    A_local[i] = distribucion(generador);
                    B_local[i] = distribucion(generador);
                }

                if (detallado)
                {
                    #pragma omp critical
                    {
                        printf(
                            "[Equipo: %s] [Proceso MPI: %d] "
                            "[Hilo OpenMP: %d] [Posicion: %d] "
                            "[A: %lld] [B: %lld] [Operacion: Crear]\n",
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

    // Suma dos secciones locales con OpenMP.
    void sumarArreglosOpenMPI(
        long long* A,
        long long* B,
        long long* R,
        int cantidad,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < cantidad; i++)
        {
            R[i] = A[i] + B[i];
            imprimirDetalle(
                hostname, mpi_rank, cantidad, i,
                "Suma", R[i], detallado
            );
        }
    }

    // Resta dos secciones locales con OpenMP.
    void restarArreglosOpenMPI(
        long long* A,
        long long* B,
        long long* R,
        int cantidad,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < cantidad; i++)
        {
            R[i] = A[i] - B[i];
            imprimirDetalle(
                hostname, mpi_rank, cantidad, i,
                "Resta", R[i], detallado
            );
        }
    }

    // Multiplica dos secciones locales elemento a elemento con OpenMP.
    void multiplicarArreglosOpenMPI(
        long long* A,
        long long* B,
        long long* R,
        int cantidad,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < cantidad; i++)
        {
            R[i] = A[i] * B[i];
            imprimirDetalle(
                hostname, mpi_rank, cantidad, i,
                "Multiplicacion", R[i], detallado
            );
        }
    }

    // Calcula el cuadrado de una seccion local con OpenMP.
    void cuadradoArregloOpenMPI(
        long long* A,
        long long* R,
        int cantidad,
        const char* hostname,
        int mpi_rank,
        bool detallado)
    {
        #pragma omp parallel for
        for (int i = 0; i < cantidad; i++)
        {
            R[i] = A[i] * A[i];
            imprimirDetalle(
                hostname, mpi_rank, cantidad, i,
                "Cuadrado", R[i], detallado
            );
        }
    }

private:
    // Centraliza el formato de la salida detallada de las operaciones.
    void imprimirDetalle(
        const char* hostname,
        int mpi_rank,
        int cantidad,
        int posicionLocal,
        const char* operacion,
        long long resultado,
        bool detallado)
    {
        if (!detallado)
        {
            return;
        }

        const int posicionGlobal =
            (mpi_rank - 1) * cantidad + posicionLocal;

        #pragma omp critical
        {
            printf(
                "[Equipo: %s] [Proceso MPI: %d] [Hilo OpenMP: %d] "
                "[Posicion: %d] [Operacion: %s] [Resultado: %lld]\n",
                hostname,
                mpi_rank,
                omp_get_thread_num(),
                posicionGlobal,
                operacion,
                resultado
            );
        }
    }
};

void imprimirIntegrantes()
{
    printf(
        "Castolo Gonzales Jorge Natanael\n"
        "Dominguez Alcala Ricardo\n"
        "Silva Montes Diego Eduardo\n"
    );
}

void mostrarArreglo(const long long* arreglo, int N)
{
    for (int i = 0; i < N; i++)
    {
        printf("%lld ", arreglo[i]);
    }
    printf("\n");
}

// Los datos utiles comienzan despues del bloque auxiliar del maestro.
void copiarDesdeBufferColectivo(
    const long long* bufferColectivo,
    long long* arregloGlobal,
    int cantidad,
    int N)
{
    for (int i = 0; i < N; i++)
    {
        arregloGlobal[i] = bufferColectivo[cantidad + i];
    }
}

const char* nombreOperacion(int opcion)
{
    switch (opcion)
    {
        case 2: return "Suma";
        case 3: return "Resta";
        case 4: return "Multiplicacion";
        case 5: return "Cuadrado";
        default: return "Desconocida";
    }
}

int main(int argc, char* argv[])
{
    int mpi_rank = 0;
    int mpi_size = 0;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int hostname_len = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Get_processor_name(hostname, &hostname_len);

    if (mpi_size < 2)
    {
        if (mpi_rank == 0)
        {
            printf("Error: se necesita al menos un maestro y un trabajador.\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (mpi_rank == 0)
    {
        imprimirIntegrantes();
        printf("\nVersion colectiva: MPI_Scatter + MPI_Gather\n\n");
    }

    const int trabajadores = mpi_size - 1;
    int tipoPrueba = 0;
    int opcion = 0;
    int N = 0;
    bool arreglosCreados = false;

    if (mpi_rank == 0)
    {
        do
        {
            printf("============================================\n");
            printf(" MPI_SCATTER + MPI_GATHER + OPENMP\n");
            printf("============================================\n");
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
                printf("Opcion invalida. Intente nuevamente.\n\n");
            }
        }
        while (tipoPrueba != 1 && tipoPrueba != 2);
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (N % trabajadores != 0)
    {
        if (mpi_rank == 0)
        {
            printf(
                "Error: %d elementos no pueden dividirse entre %d trabajadores.\n",
                N,
                trabajadores
            );
        }
        MPI_Finalize();
        return 1;
    }

    const int cantidad = N / trabajadores;
    const int cantidadColectiva = mpi_size * cantidad;
    const bool detallado = N == 40;

    // Todos los procesos necesitan un bloque local. En MPI 0 este bloque es
    // auxiliar y nunca se procesa; en los demas contiene una seccion real.
    long long* A_local = new long long[cantidad]();
    long long* B_local = new long long[cantidad]();
    long long* R_local = new long long[cantidad]();

    long long* A = nullptr;
    long long* B = nullptr;
    long long* R = nullptr;
    long long* A_colectivo = nullptr;
    long long* B_colectivo = nullptr;
    long long* R_colectivo = nullptr;

    if (mpi_rank == 0)
    {
        A = new long long[N];
        B = new long long[N];
        R = new long long[N];

        // El primer bloque de cada buffer corresponde al maestro y permanece
        // vacio. Los cuatro bloques siguientes corresponden a los trabajadores.
        A_colectivo = new long long[cantidadColectiva]();
        B_colectivo = new long long[cantidadColectiva]();
        R_colectivo = new long long[cantidadColectiva]();
    }

    if (detallado)
    {
        printf(
            "[Equipo: %s] [Proceso MPI: %d de %d] [%s]\n",
            hostname,
            mpi_rank,
            mpi_size,
            mpi_rank == 0 ? "MAESTRO" : "TRABAJADOR"
        );
    }

    OperacionesArreglos operaciones;

    do
    {
        if (mpi_rank == 0)
        {
            printf("\n============================================\n");
            printf(" MENU - VERSION SCATTER/GATHER\n");
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

        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (opcion == 1)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            const double inicio = MPI_Wtime();

            if (mpi_rank != 0)
            {
                operaciones.crearArregloMPI(
                    A_local,
                    B_local,
                    cantidad,
                    N,
                    hostname,
                    mpi_rank,
                    detallado
                );

                if (detallado)
                {
                    const int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[Participacion: MPI_Gather] [Seccion: %d - %d]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1
                    );
                }
            }

            // Cada proceso aporta un bloque. El bloque de MPI 0 es auxiliar.
            MPI_Gather(
                A_local,
                cantidad,
                MPI_LONG_LONG,
                A_colectivo,
                cantidad,
                MPI_LONG_LONG,
                0,
                MPI_COMM_WORLD
            );
            MPI_Gather(
                B_local,
                cantidad,
                MPI_LONG_LONG,
                B_colectivo,
                cantidad,
                MPI_LONG_LONG,
                0,
                MPI_COMM_WORLD
            );

            if (mpi_rank == 0)
            {
                copiarDesdeBufferColectivo(A_colectivo, A, cantidad, N);
                copiarDesdeBufferColectivo(B_colectivo, B, cantidad, N);

                const double fin = MPI_Wtime();

                if (detallado)
                {
                    for (int origen = 1; origen < mpi_size; origen++)
                    {
                        const int inicioSeccion = (origen - 1) * cantidad;
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[MPI_Gather recibido de: %d] [Seccion: %d - %d]\n",
                            hostname,
                            origen,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1
                        );
                    }

                    printf("\nArreglo A:\n");
                    mostrarArreglo(A, N);
                    printf("\nArreglo B:\n");
                    mostrarArreglo(B, N);
                }

                printf(
                    "[Equipo: %s] [Proceso MPI: 0] "
                    "[Tiempo creacion con MPI_Gather: %.10f segundos]\n",
                    hostname,
                    fin - inicio
                );
            }

            arreglosCreados = true;
        }
        else if (opcion >= 2 && opcion <= 5)
        {
            if (!arreglosCreados)
            {
                if (mpi_rank == 0)
                {
                    printf("Primero debe crear los arreglos con la opcion 1.\n");
                }
                continue;
            }

            const char* operacion = nombreOperacion(opcion);
            const bool necesitaB = opcion != 5;

            MPI_Barrier(MPI_COMM_WORLD);
            const double inicio = MPI_Wtime();

            if (mpi_rank == 0 && detallado)
            {
                for (int destino = 1; destino < mpi_size; destino++)
                {
                    const int inicioSeccion = (destino - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: 0] "
                        "[MPI_Scatter hacia: %d] [Seccion: %d - %d] "
                        "[Operacion: %s]\n",
                        hostname,
                        destino,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1,
                        operacion
                    );
                }
            }

            // El maestro distribuye A. Su primer bloque es auxiliar.
            MPI_Scatter(
                A_colectivo,
                cantidad,
                MPI_LONG_LONG,
                A_local,
                cantidad,
                MPI_LONG_LONG,
                0,
                MPI_COMM_WORLD
            );

            if (necesitaB)
            {
                MPI_Scatter(
                    B_colectivo,
                    cantidad,
                    MPI_LONG_LONG,
                    B_local,
                    cantidad,
                    MPI_LONG_LONG,
                    0,
                    MPI_COMM_WORLD
                );
            }

            if (mpi_rank != 0)
            {
                if (detallado)
                {
                    const int inicioSeccion = (mpi_rank - 1) * cantidad;
                    printf(
                        "[Equipo: %s] [Proceso MPI: %d] "
                        "[MPI_Scatter recibido] [Seccion: %d - %d] "
                        "[Operacion: %s]\n",
                        hostname,
                        mpi_rank,
                        inicioSeccion,
                        inicioSeccion + cantidad - 1,
                        operacion
                    );
                }

                switch (opcion)
                {
                    case 2:
                        operaciones.sumarArreglosOpenMPI(
                            A_local, B_local, R_local, cantidad,
                            hostname, mpi_rank, detallado
                        );
                        break;
                    case 3:
                        operaciones.restarArreglosOpenMPI(
                            A_local, B_local, R_local, cantidad,
                            hostname, mpi_rank, detallado
                        );
                        break;
                    case 4:
                        operaciones.multiplicarArreglosOpenMPI(
                            A_local, B_local, R_local, cantidad,
                            hostname, mpi_rank, detallado
                        );
                        break;
                    case 5:
                        operaciones.cuadradoArregloOpenMPI(
                            A_local, R_local, cantidad,
                            hostname, mpi_rank, detallado
                        );
                        break;
                }
            }

            // El bloque de MPI 0 sigue siendo auxiliar; el maestro copia solo
            // los cuatro bloques de resultados de los trabajadores.
            MPI_Gather(
                R_local,
                cantidad,
                MPI_LONG_LONG,
                R_colectivo,
                cantidad,
                MPI_LONG_LONG,
                0,
                MPI_COMM_WORLD
            );

            if (mpi_rank == 0)
            {
                copiarDesdeBufferColectivo(R_colectivo, R, cantidad, N);
                const double fin = MPI_Wtime();

                if (detallado)
                {
                    for (int origen = 1; origen < mpi_size; origen++)
                    {
                        const int inicioSeccion = (origen - 1) * cantidad;
                        printf(
                            "[Equipo: %s] [Proceso MPI: 0] "
                            "[MPI_Gather recibido de: %d] [Seccion: %d - %d] "
                            "[Operacion: %s]\n",
                            hostname,
                            origen,
                            inicioSeccion,
                            inicioSeccion + cantidad - 1,
                            operacion
                        );
                    }

                    printf("\nResultado %s:\n", operacion);
                    mostrarArreglo(R, N);
                }

                printf(
                    "[Equipo: %s] [Proceso MPI: 0] "
                    "[Tiempo %s con Scatter/Gather: %.10f segundos]\n",
                    hostname,
                    operacion,
                    fin - inicio
                );
            }
        }
        else if (opcion == 6)
        {
            if (mpi_rank == 0)
            {
                printf("Saliendo del programa...\n\n");
            }
        }
        else if (mpi_rank == 0)
        {
            printf("Opcion invalida. Seleccione una opcion del 1 al 6.\n");
        }
    }
    while (opcion != 6);

    delete[] A_local;
    delete[] B_local;
    delete[] R_local;

    if (mpi_rank == 0)
    {
        delete[] A;
        delete[] B;
        delete[] R;
        delete[] A_colectivo;
        delete[] B_colectivo;
        delete[] R_colectivo;
    }

    // Sincroniza la ultima salida para que los nombres cierren la ejecucion.
    MPI_Barrier(MPI_COMM_WORLD);
    if (mpi_rank == 0)
    {
        imprimirIntegrantes();
    }

    MPI_Finalize();
    return 0;
}
