#include "ArregloDinamico.h"
#include "Busquedas.h"
#include "Registro.h"

#include <mpi.h>
#include <omp.h>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
const char* INTEGRANTES = "Castolo Gonzales Jorge Natanael | Dominguez Alcala Ricardo | Silva Montes Diego Eduardo";

struct Configuracion {
    int n = 1000000;
    int maximo = 1000000;
    int target = 42;
    bool targetFijo = false;
    bool detallado = false;
    std::string logs = "logs";
};

struct MedicionBusqueda {
    bool disponible = false;
    double local = 0.0;
    double distribuida = 0.0;
    double comunicacion = 0.0;
    long long posicionLocal = -1;
    long long posicionDistribuida = -1;
    int target = 0;
};

long long convertir(const std::string& texto, long long minimo, long long maximo) {
    std::size_t usados = 0;
    const long long valor = std::stoll(texto, &usados);
    if (usados != texto.size() || valor < minimo || valor > maximo)
        throw std::runtime_error("Numero fuera de rango: " + texto);
    return valor;
}

Configuracion leerArgumentos(int argc, char** argv) {
    Configuracion c;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto valor = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error("Falta valor para " + arg);
            return argv[i];
        };
        if (arg == "--n") c.n = static_cast<int>(convertir(valor(), 1, INT_MAX));
        else if (arg == "--max") c.maximo = static_cast<int>(convertir(valor(), 1, INT_MAX));
        else if (arg == "--target") {
            c.target = static_cast<int>(convertir(valor(), INT_MIN, INT_MAX));
            c.targetFijo = true;
        } else if (arg == "--logs") c.logs = valor();
        else if (arg == "--detallado") c.detallado = true;
        else if (arg == "--silencioso") c.detallado = false;
        else throw std::runtime_error("Argumento desconocido: " + arg);
    }
    return c;
}

std::string numero(double valor) {
    std::ostringstream salida;
    salida << std::fixed << std::setprecision(9) << valor;
    return salida.str();
}

void crearDistribucion(int n, int procesos, int* cantidades, int* desplazamientos) {
    const int base = n / procesos;
    const int residuo = n % procesos;
    int desplazamiento = 0;
    for (int r = 0; r < procesos; ++r) {
        cantidades[r] = base + (r < residuo ? 1 : 0);
        desplazamientos[r] = desplazamiento;
        desplazamiento += cantidades[r];
    }
}

void llenarAleatorio(ArregloDinamico& arreglo, unsigned semilla, int maximo) {
    #pragma omp parallel
    {
        unsigned estado = semilla ^ (0x9e3779b9U * static_cast<unsigned>(omp_get_thread_num() + 1));
        #pragma omp for schedule(static)
        for (long long i = 0; i < static_cast<long long>(arreglo.cantidad()); ++i) {
            estado = estado * 1664525U + 1013904223U;
            arreglo.datos()[i] = static_cast<int>(estado % (static_cast<unsigned>(maximo) + 1U));
        }
    }
}

int leerOpcion(Registro& log) {
    int opcion = 0;
    log.general("MENU - Actividad 1.5");
    log.general("1. Generar Arreglo Dinamico");
    log.general("2. Ordenar Arreglo con OpenMP");
    log.general("3. Busqueda Secuencial (Local vs Distribuida)");
    log.general("4. Busqueda Binaria (Local vs Distribuida)");
    log.general("5. Mostrar Tiempos y Comparativa");
    log.general("6. Salir");
    log.general("Selecciona una opcion (1-6):");
    if (!(std::cin >> opcion)) return 6;
    return opcion >= 1 && opcion <= 6 ? opcion : 0;
}

int leerTarget(const Configuracion& c) {
    if (c.targetFijo) return c.target;
    int target = 0;
    std::cout << "Valor entero a buscar: " << std::flush;
    if (!(std::cin >> target)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return 0;
    }
    return target;
}

void imprimirComparacion(Registro& log, const std::string& algoritmo,
                         const MedicionBusqueda& m, int hilosTotales) {
    if (!m.disponible) {
        log.general("[Comparativa: " + algoritmo + "] Sin mediciones.");
        return;
    }
    const double speedup = m.distribuida > 0.0 ? m.local / m.distribuida : 0.0;
    const double eficiencia = hilosTotales > 0 ? speedup / hilosTotales : 0.0;
    log.general("[Comparativa: " + algoritmo + "] [Target: " + std::to_string(m.target) +
                "] [Local OpenMP: " + numero(m.local) + " s] [Distribuida MPI+OpenMP: " +
                numero(m.distribuida) + " s] [Comunicacion: " + numero(m.comunicacion) +
                " s] [Speedup: " + numero(speedup) + "] [Eficiencia: " +
                numero(eficiencia) + "]");
}

MedicionBusqueda ejecutarBusqueda(bool binaria, const Configuracion& c,
                                  ArregloDinamico& global, ArregloDinamico& local,
                                  int* cantidades, int* desplazamientos,
                                  int rank, int target,
                                  Registro& log) {
    MedicionBusqueda m;
    m.disponible = true;
    m.target = target;
    const std::string nombreLocal = binaria ? "Binaria Local OpenMP" : "Secuencial Local OpenMP";
    const std::string nombreDistribuido = binaria ? "Binaria Distribuida MPI+OpenMP"
                                                   : "Secuencial Distribuida MPI+OpenMP";

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        const double inicio = MPI_Wtime();
        m.posicionLocal = binaria
            ? busquedaBinariaOpenMP(global.datos(), global.cantidad(), target, 0, log,
                                    nombreLocal, c.detallado)
            : busquedaSecuencialOpenMP(global.datos(), global.cantidad(), target, 0, log,
                                       nombreLocal, c.detallado);
        m.local = MPI_Wtime() - inicio;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    const double inicioTotal = MPI_Wtime();
    const double inicioScatter = MPI_Wtime();
    MPI_Scatterv(rank == 0 ? global.datos() : nullptr, cantidades, desplazamientos, MPI_INT,
                 local.datos(), cantidades[rank], MPI_INT, 0, MPI_COMM_WORLD);
    const double finScatter = MPI_Wtime();

    long long resultadoLocal = -1;
    if (c.detallado) {
        // Las pruebas detalladas priorizan trazabilidad legible. Cada proceso
        // presenta el trabajo de sus hilos por turno para evitar líneas mezcladas.
        int procesos = 1;
        MPI_Comm_size(MPI_COMM_WORLD, &procesos);
        for (int turno = 0; turno < procesos; ++turno) {
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == turno) {
                resultadoLocal = binaria
                    ? busquedaBinariaOpenMP(local.datos(), local.cantidad(), target,
                                            desplazamientos[rank], log, nombreDistribuido, true)
                    : busquedaSecuencialOpenMP(local.datos(), local.cantidad(), target,
                                               desplazamientos[rank], log, nombreDistribuido, true);
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    } else {
        resultadoLocal = binaria
            ? busquedaBinariaOpenMP(local.datos(), local.cantidad(), target,
                                    desplazamientos[rank], log, nombreDistribuido, false)
            : busquedaSecuencialOpenMP(local.datos(), local.cantidad(), target,
                                       desplazamientos[rank], log, nombreDistribuido, false);
    }
    const long long candidato = resultadoLocal < 0 ? LLONG_MAX : resultadoLocal;
    long long resultadoGlobal = LLONG_MAX;
    const double inicioReduce = MPI_Wtime();
    MPI_Reduce(&candidato, &resultadoGlobal, 1, MPI_LONG_LONG_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    const double finTotal = MPI_Wtime();

    const double tiempoLocalTotal = finTotal - inicioTotal;
    const double tiempoLocalComunicacion = (finScatter - inicioScatter) + (finTotal - inicioReduce);
    MPI_Reduce(&tiempoLocalTotal, &m.distribuida, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tiempoLocalComunicacion, &m.comunicacion, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        m.posicionDistribuida = resultadoGlobal == LLONG_MAX ? -1 : resultadoGlobal;
        if (m.posicionLocal != m.posicionDistribuida)
            throw std::runtime_error("Los esquemas local y distribuido devolvieron indices diferentes");
        log.general("[Resultado: " + nombreDistribuido + "] [Target: " +
                    std::to_string(target) + "] [Indice global: " +
                    std::to_string(m.posicionDistribuida) + "] [Tiempo local: " +
                    numero(m.local) + " s] [Tiempo distribuido: " +
                    numero(m.distribuida) + " s]");
    }
    return m;
}
}

int main(int argc, char** argv) {
    int soporte = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &soporte);
    int rank = 0, procesos = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);

    char* nombre = new char[MPI_MAX_PROCESSOR_NAME]{};
    int longitud = 0;
    MPI_Get_processor_name(nombre, &longitud);
    const std::string equipo(nombre, longitud);
    delete[] nombre;

    try {
        if (soporte < MPI_THREAD_FUNNELED)
            throw std::runtime_error("MPI no proporciona MPI_THREAD_FUNNELED");
        Configuracion c = leerArgumentos(argc, argv);
        Registro log(c.logs, equipo, rank);
        log.general(std::string("[Integrantes: ") + INTEGRANTES + ']');
        log.general("[Inicio] [Procesos MPI: " + std::to_string(procesos) +
                    "] [Hilos OpenMP: " + std::to_string(omp_get_max_threads()) +
                    "] [N configurado: " + std::to_string(c.n) +
                    "] [Log: " + log.ruta() + "]");

        int* cantidades = new int[procesos];
        int* desplazamientos = new int[procesos];
        crearDistribucion(c.n, procesos, cantidades, desplazamientos);
        ArregloDinamico global;
        ArregloDinamico local(static_cast<std::size_t>(cantidades[rank]));
        bool generado = false, ordenado = false;
        double tiempoOrdenSecuencial = 0.0, tiempoOrdenOpenMP = 0.0;
        MedicionBusqueda secuencial, binaria;

        int hilosLocales = omp_get_max_threads();
        int hilosTotales = 0;
        MPI_Reduce(&hilosLocales, &hilosTotales, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        bool continuar = true;
        while (continuar) {
            int opcion = rank == 0 ? leerOpcion(log) : 0;
            MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (opcion == 0) continue;
            if (opcion == 1) {
                if (rank == 0) {
                    global.reservar(static_cast<std::size_t>(c.n));
                    const unsigned semilla = static_cast<unsigned>(std::time(nullptr));
                    llenarAleatorio(global, semilla, c.maximo);
                    log.general("[Generacion] Arreglo dinamico creado con new[]; N=" +
                                std::to_string(c.n) + "; rango=0.." + std::to_string(c.maximo));
                }
                generado = true;
                ordenado = false;
                secuencial.disponible = false;
                binaria.disponible = false;
            } else if (opcion == 2) {
                if (!generado) {
                    if (rank == 0) log.general("Primero genera el arreglo con la opcion 1.");
                    continue;
                }
                if (rank == 0) {
                    ArregloDinamico copia;
                    copia.copiarDesde(global);
                    ArregloDinamico auxiliarSecuencial(global.cantidad());
                    ArregloDinamico auxiliarParalelo(global.cantidad());
                    double inicio = MPI_Wtime();
                    ordenarMergeSecuencial(copia.datos(), copia.cantidad(), auxiliarSecuencial.datos());
                    tiempoOrdenSecuencial = MPI_Wtime() - inicio;
                    inicio = MPI_Wtime();
                    ordenarMergeOpenMP(global.datos(), global.cantidad(), auxiliarParalelo.datos());
                    tiempoOrdenOpenMP = MPI_Wtime() - inicio;
                    if (!estaOrdenado(global.datos(), global.cantidad()) ||
                        !std::equal(global.datos(), global.datos() + global.cantidad(), copia.datos()))
                        throw std::runtime_error("El ordenamiento paralelo no coincide con el secuencial");
                    log.general("[Ordenamiento validado] [Secuencial: " + numero(tiempoOrdenSecuencial) +
                                " s] [Parallel Merge Sort OpenMP: " + numero(tiempoOrdenOpenMP) +
                                " s] [Speedup: " + numero(tiempoOrdenOpenMP > 0.0
                                    ? tiempoOrdenSecuencial / tiempoOrdenOpenMP : 0.0) + "]");
                }
                ordenado = true;
            } else if (opcion == 3 || opcion == 4) {
                if (!generado || (opcion == 4 && !ordenado)) {
                    if (rank == 0) log.general(opcion == 4
                        ? "Primero genera y ordena el arreglo con las opciones 1 y 2."
                        : "Primero genera el arreglo con la opcion 1.");
                    continue;
                }
                int target = rank == 0 ? leerTarget(c) : 0;
                MPI_Bcast(&target, 1, MPI_INT, 0, MPI_COMM_WORLD);
                MedicionBusqueda resultado = ejecutarBusqueda(opcion == 4, c, global, local,
                    cantidades, desplazamientos, rank, target, log);
                if (opcion == 3) secuencial = resultado;
                else binaria = resultado;
            } else if (opcion == 5) {
                if (rank == 0) {
                    if (tiempoOrdenOpenMP > 0.0)
                        log.general("[Comparativa: Ordenamiento] [Secuencial: " +
                            numero(tiempoOrdenSecuencial) + " s] [OpenMP: " +
                            numero(tiempoOrdenOpenMP) + " s] [Speedup: " +
                            numero(tiempoOrdenSecuencial / tiempoOrdenOpenMP) + "]");
                    else log.general("[Comparativa: Ordenamiento] Sin mediciones.");
                    imprimirComparacion(log, "Busqueda Secuencial", secuencial, hilosTotales);
                    imprimirComparacion(log, "Busqueda Binaria", binaria, hilosTotales);
                }
            } else if (opcion == 6) {
                continuar = false;
            }
        }

        log.general("[Fin] Memoria dinamica liberada antes de MPI_Finalize.");
        delete[] cantidades;
        delete[] desplazamientos;
        MPI_Finalize();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Equipo: " << equipo << "] [Nodo MPI: " << rank
                  << "] ERROR: " << error.what() << '\n';
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
}
