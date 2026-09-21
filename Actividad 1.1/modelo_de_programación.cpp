#include <mpi.h>
#include <omp.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

class ArregloDinamico { // claese
    std::size_t n_; 
    int* datos_;

public:
    explicit ArregloDinamico(std::size_t n) // constructor
        : n_(n), datos_(new int[n]) {}

    ~ArregloDinamico() { // destructor
        delete[] datos_;
    }

    ArregloDinamico(const ArregloDinamico&) = delete;
    ArregloDinamico& operator=(const ArregloDinamico&) = delete;

    void llenar(int rango, int procesos, const char* nodo, const char* equipo) {
        // Cada fase termina al alcanzar un umbral real del arreglo.
        #pragma omp parallel
        {
            const int hilo = omp_get_thread_num();
            const int hilos = omp_get_num_threads();
            unsigned long long escritos = 0;

            for (int fase = 1; fase <= 10; ++fase) {
                const std::size_t inicio =
                    (n_ * (fase - 1) + 9) / 10;
                const std::size_t fin =
                    (n_ * fase + 9) / 10;

                // En casos pequeños hay una iteración por elemento.
                const std::size_t bloque = n_ < 1000 ? 1 : 1024;
                const long long bloques = static_cast<long long>(
                    (fin - inicio + bloque - 1) / bloque
                );

                #pragma omp for schedule(static)
                for (long long b = 0; b < bloques; ++b) {
                    // Auxiliar fijo; datos_ es el almacenamiento dinámico.
                    int temporal[1024];

                    const std::size_t desde =
                        inicio + static_cast<std::size_t>(b) * bloque;
                    const std::size_t hasta = std::min(desde + bloque, fin);

                    #pragma omp critical(generador_aleatorio)
                    {
                        for (std::size_t j = 0; j < hasta - desde; ++j) {
                            temporal[j] = std::rand() % 100;
                        }
                    }

                    for (std::size_t i = desde; i < hasta; ++i) {
                        datos_[i] = temporal[i - desde];
                    }

                    escritos += hasta - desde;
                }

                // Barrera implícita: todos los elementos hasta fin ya están escritos.
                #pragma omp single
                {
                    std::printf(
                        "AVANCE %d%% proceso=%d/%d nodo=%s equipo=%s "
                        "hilo=%d hilos=%d llenados=%zu/%zu\n",
                        fase * 10,
                        rango,
                        procesos,
                        nodo,
                        equipo,
                        hilo,
                        hilos,
                        fin,
                        n_
                    );
                    std::fflush(stdout);
                }
            }

            #pragma omp critical(salida_hilos)
            {
                std::printf(
                    "TRABAJO proceso=%d nodo=%s equipo=%s "
                    "hilo=%d elementos=%llu\n",
                    rango,
                    nodo,
                    equipo,
                    hilo,
                    escritos
                );
            }
        }
    }

    unsigned long long verificar() const {
        unsigned long long suma = 0;
        
        int invalido = 0;

        #pragma omp parallel for reduction(+:suma) reduction(|:invalido)
        for (long long i = 0; i < static_cast<long long>(n_); ++i) {
            invalido |= datos_[i] < 0 || datos_[i] > 99;
            suma += static_cast<unsigned>(datos_[i]);
        }

        if (invalido) {
            throw std::runtime_error("Valor fuera de rango");
        }

        return suma;
    }

    void mostrar(
        std::ostream& out,
        int rango,
        int procesos,
        const char* nodo,
        const char* equipo
    ) const {
        out << "CONTENIDO proceso=" << rango << '/' << procesos
            << " nodo=" << nodo
            << " equipo=" << equipo
            << " elementos=" << n_ << '\n';

        for (std::size_t i = 0; i < n_; ++i) {
            out << datos_[i] << ((i + 1) % 25 == 0 ? '\n' : ' ');
        }

        out << '\n';

        if (!out) {
            throw std::runtime_error("No se pudo escribir el contenido completo");
        }
    }
};

int main(int argc, char** argv) {
    // Las llamadas MPI se realizan exclusivamente en el hilo principal.
#ifdef OPEN_MPI
    if (setenv("OMPI_MPI_THREAD_LEVEL", "1", 1) != 0) {
        return 1;
    }
    MPI_Init(&argc, &argv); // inicializacion de MPI 
#else
    int solicitado; // OpenMP puede crear múltiples hilos.
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &solicitado); // solo el hilo prinicipal realiza llamadas mpi
#endif 

    int rango;
    int procesos;
    int longitud;
    int nivel;
    char nodo[MPI_MAX_PROCESSOR_NAME];
    std::string equipo = "Equipo-MPI-OpenMP";

    MPI_Comm_rank(MPI_COMM_WORLD, &rango); // identificador del proceso
    /*
     - Identificar los mensajes de avance.
     - Crear una semilla diferente.
     - Crear un archivo diferente por proceso.
     - Ordenar la impresión de los arreglos.
    */
    MPI_Comm_size(MPI_COMM_WORLD, &procesos); // cuantos procesos hay ejecutandose en ese momento lanzados por MPI
    MPI_Get_processor_name(nodo, &longitud); // nodo o equipo | el nombre de la computadora donde está ejecutándose el proceso.
    nodo[longitud] = '\0';
    MPI_Query_thread(&nivel);

    try {
        if (nivel < MPI_THREAD_FUNNELED) {
            throw std::runtime_error("MPI no ofrece soporte FUNNELED");
        }

        bool pequeno = true;
        bool archivos = false;
        unsigned long long n = 0;
        std::string carpeta = ".";

        for (int a = 1; a < argc; ++a) {
            const std::string arg = argv[a];

            if (arg == "--pequeno") {
                pequeno = true;
            } else if (arg == "--solo-archivos") {
                archivos = true;
            } else if (arg == "--salida" && a + 1 < argc) {
                carpeta = argv[++a];
            } else if (arg == "--equipo" && a + 1 < argc) {
                equipo = argv[++a];
            } else if (arg == "--n" && a + 1 < argc) {
                const char* texto = argv[++a];
                char* fin = nullptr;
                errno = 0;

                n = std::strtoull(texto, &fin, 10);

                if (
                    errno ||
                    texto[0] == '-' ||
                    fin == texto ||
                    *fin ||
                    n < 10 ||
                    n > std::numeric_limits<std::size_t>::max() / 10 ||
                    n > static_cast<unsigned long long>(
                        std::numeric_limits<long long>::max()
                    )
                ) {
                    throw std::runtime_error(
                        "Tamano invalido (minimo 10 elementos)"
                    );
                }

                pequeno = false;
            } else {
                throw std::runtime_error(
                    "Uso: --pequeno | --n N [--equipo NOMBRE] "
                    "[--salida CARPETA] [--solo-archivos]"
                );
            }
        }

        unsigned semilla_base = 0;

        if (rango == 0) {
            semilla_base = static_cast<unsigned>(std::time(nullptr));
        }

        MPI_Bcast( //  distribuye la semilla base entre los procesos
            &semilla_base, // semilla en base al tiemp o hora y se empieza a sumar para no ser el mismo y no cometer la misma secuenia
            1, // 1 solo elemento o semilla
            MPI_UNSIGNED, // entero mpi sin signo
            0, // el 0 es el emisor 
            MPI_COMM_WORLD // hacia todos los procesos!
        );

        const unsigned semilla =
            semilla_base + static_cast<unsigned>(rango);

        std::srand(semilla);

        if (pequeno) {
            n = 20 + std::rand() % 31;
        }

        omp_set_dynamic(0);

        // Permite repartir los núcleos entre procesos con OMP_NUM_THREADS.
        if (std::getenv("OMP_NUM_THREADS") == nullptr) {
            omp_set_num_threads(omp_get_num_procs());
        }

        std::printf(
            "INICIO proceso=%d/%d nodo=%s equipo=%s n=%llu "
            "semilla=%u hilos_solicitados=%d\n",
            rango,
            procesos,
            nodo,
            equipo.c_str(),
            n,
            semilla,
            omp_get_max_threads()
        );

        {
            ArregloDinamico arreglo(static_cast<std::size_t>(n));

            MPI_Barrier(MPI_COMM_WORLD); // barrera de sincronizacion 

            const double inicio = MPI_Wtime(); // inicio contador 
            arreglo.llenar(rango, procesos, nodo, equipo.c_str());
            const double segundos = MPI_Wtime() - inicio; // termino
            // tiempo total de llendo en segundos.
            // esto es para que cada proceso puede medir su tiempo de llando
            const auto suma = arreglo.verificar();
            const std::string ruta =
                carpeta + "/arreglo-proceso-" + std::to_string(rango) + ".txt";

            std::ofstream salida(ruta);

            if (!salida) {
                throw std::runtime_error("No se pudo abrir " + ruta);
            }

            arreglo.mostrar(salida, rango, procesos, nodo, equipo.c_str());
            salida.close();

            if (!salida) {
                throw std::runtime_error("Error al cerrar " + ruta);
            }

            std::printf(
                "RESULTADO proceso=%d nodo=%s equipo=%s n=%llu "
                "suma=%llu segundos_llenado=%.6f archivo=%s\n",
                rango,
                nodo,
                equipo.c_str(),
                n,
                suma,
                segundos,
                ruta.c_str()
            );

            for (int turno = 0; turno < procesos; ++turno) {
                MPI_Barrier(MPI_COMM_WORLD); // barrera de sincronizacion

                if (turno == rango && !archivos) {
                    std::ifstream entrada(ruta);
                    char linea[256];

                    while (entrada.getline(linea, sizeof(linea))) {
                        std::puts(linea);
                    }

                    std::fflush(stdout);
                }
            }

            unsigned long long total = 0;

            MPI_Reduce( // realiza operacion
                &n, // valor que trabajo el proceso
                &total, // se entrega a total el resultado
                1, // 1 dato por proceso
                MPI_UNSIGNED_LONG_LONG, // tipo de dato long long | 10 millones
                MPI_SUM, // funcion suma 
                0, // se va al proceso 0 el resultado
                MPI_COMM_WORLD // Participan todos los procesos en la actividad o tarea 
            );

            if (rango == 0) {
                std::printf(
                    "FINAL procesos=%d elementos_totales=%llu\n",
                    procesos,
                    total
                );
            }
        } // delete[] antes de MPI_Finalize.

        MPI_Finalize(); // Finaliza correctamente el entorno MPI. | arreglo destruido y memoria liberada
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(
            stderr,
            "ERROR proceso=%d nodo=%s equipo=%s: %s\n",
            rango,
            nodo,
            equipo.c_str(),
            error.what()
        );

        MPI_Abort(MPI_COMM_WORLD, 1); // en el caso de ocurrir un error 
        return 1;
    }
}
