/*
Castolo Gonzales Jorge Natanael
Dominguez Alcala Ricardo
Silva Montes Diego Eduardo
Actividad 1.3: arreglos distribuidos, reducciones MPI y OpenMP.
*/
#include "Comunicacion.h"
#include <omp.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#ifndef VERSION_PREDETERMINADA
#define VERSION_PREDETERMINADA 3
#endif

namespace {
const char* integrantes = "Castolo Gonzales Jorge Natanael | Dominguez Alcala Ricardo | Silva Montes Diego Eduardo";

struct Configuracion {
    int n = 40;
    Version version = VERSION_PREDETERMINADA == 1 ? Version::SendRecv
                    : VERSION_PREDETERMINADA == 2 ? Version::ScatterGather : Version::Reduce;
    int colectiva = 0; // 0: preguntar, 1: Reduce, 2: Allreduce
    int detalle = -1; // automatico: detallado cuando N <= 40
    bool ayuda = false;
    std::string logs = "logs";
};

// Propietario de los punteros: liberacion automatica tambien ante excepciones.
struct Arreglos {
    Entero *A = nullptr, *B = nullptr, *C = nullptr;
    ~Arreglos() { delete[] A; delete[] B; delete[] C; }
    void crear(std::size_t cantidad) {
        delete[] A; delete[] B; delete[] C;
        A = nullptr; B = nullptr; C = nullptr;
        A = new Entero[cantidad]{};
        B = new Entero[cantidad]{};
        C = new Entero[cantidad]{};
    }
};

long long numero(const std::string& texto, long long minimo, long long maximo) {
    std::size_t fin = 0;
    const long long valor = std::stoll(texto, &fin);
    if (fin != texto.size() || valor < minimo || valor > maximo)
        throw std::runtime_error("Numero fuera de rango: " + texto);
    return valor;
}

Configuracion argumentos(int argc, char** argv) {
    Configuracion c;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto valor = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error("Falta valor para " + arg);
            return argv[i];
        };
        if (arg == "--n") c.n = static_cast<int>(numero(valor(), 1, std::numeric_limits<int>::max()));
        else if (arg == "--logs") c.logs = valor();
        else if (arg == "--colectiva") {
            const std::string v = valor();
            if (v == "reduce") c.colectiva = 1;
            else if (v == "allreduce") c.colectiva = 2;
            else throw std::runtime_error("Colectiva valida: reduce o allreduce");
        } else if (arg == "--detallado") c.detalle = 1;
        else if (arg == "--silencioso") c.detalle = 0;
        else if (arg == "--help" || arg == "-h") c.ayuda = true;
        else throw std::runtime_error("Argumento desconocido: " + arg);
    }
    return c;
}

std::string decimal(double valor) {
    std::ostringstream salida;
    salida << std::fixed << std::setprecision(9) << valor;
    return salida.str();
}

std::string fecha() {
    const std::time_t ahora = std::time(nullptr);
    const std::tm local = *std::localtime(&ahora);
    std::ostringstream salida;
    salida << std::put_time(&local, "%Y-%m-%d %H:%M:%S %Z");
    return salida.str();
}

int leerEntero(Registro& log, const std::string& pregunta, int minimo, int maximo, int eof) {
    while (true) {
        log.escribir(pregunta);
        std::string linea;
        if (!std::getline(std::cin, linea)) {
            log.escribir("Fin de entrada; se solicita salir.");
            return eof;
        }
        log.escribir("Entrada: " + linea, false);
        try { return static_cast<int>(numero(linea, minimo, maximo)); }
        catch (const std::exception&) { log.escribir("Entrada invalida; ingresa un entero en el rango solicitado."); }
    }
}

const char* nombreOperacion(int opcion) {
    switch (opcion) {
        case 1: return "Crear arreglos";
        case 2: return "Suma";
        case 3: return "Resta";
        case 4: return "Multiplicacion";
        case 5: return "Cuadrado";
        case 6: return "Llenar secuencial";
        case 7: return "Llenar aleatorio";
        case 8: return "Sumatoria";
        case 9: return "Promedio";
        case 10: return "Maximo";
        case 11: return "Minimo";
        default: return "Salir";
    }
}

void menu(Registro& log) {
    log.escribir("MENU - Actividad 1.3");
    for (int op = 1; op <= 12; ++op)
        log.escribir(std::to_string(op) + ". " + nombreOperacion(op));
}

void ayuda(Registro& log) {
    log.escribir("Uso: mpirun -np 5 ./arreglos_reduce [opciones]");
    log.escribir("--n N (40 por defecto); --logs DIRECTORIO");
    log.escribir("--colectiva reduce|allreduce (sin ella se pregunta en el menu)");
    log.escribir("--detallado; --silencioso; --help");
}
}

int main(int argc, char** argv) {
    // MPI_Init_thread inicializa MPI y solicita soporte seguro para OpenMP.
    // Todas las llamadas MPI se ejecutan exclusivamente en este hilo principal.
    int soporte = 0;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &soporte);
    int rank = 0, procesos = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);
    char* nombre = new char[MPI_MAX_PROCESSOR_NAME + 1]{};
    int longitud = 0;
    MPI_Get_processor_name(nombre, &longitud);
    const std::string equipo(nombre, longitud);
    delete[] nombre;
    Registro* registro = nullptr;
    int estado = 0;
    try {
        const Configuracion c = argumentos(argc, argv);
        registro = new Registro(c.logs, equipo, rank);
        // Las cabeceras de los trabajadores se presentan desde el maestro.
        registro->escribir(integrantes);
        registro->mostrarPendientes();
        if (soporte < MPI_THREAD_FUNNELED) throw std::runtime_error("MPI no ofrece MPI_THREAD_FUNNELED");
        if (c.ayuda) {
            if (rank == 0) ayuda(*registro);
        } else if (procesos < 2) {
            if (rank == 0) registro->escribir("Se necesitan al menos 2 procesos: 1 maestro y 1 trabajador.");
            estado = 1;
        } else {
            const bool detallado = c.detalle < 0 ? c.n <= 40 : c.detalle == 1;
            const int trabajadores = procesos - 1;
            const int bloque = static_cast<int>((static_cast<long long>(c.n) + trabajadores - 1) / trabajadores);
            const long long inicioLargo = rank == 0 ? 0 : static_cast<long long>(rank - 1) * bloque;
            const int cantidad = rank == 0 || inicioLargo >= c.n ? 0
                               : static_cast<int>(std::min(static_cast<long long>(bloque), c.n - inicioLargo));
            const int inicio = static_cast<int>(std::min(inicioLargo, static_cast<long long>(c.n)));
            unsigned int base = 0;
            if (rank == 0) {
                base = static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            }
            MPI_Bcast(&base, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
            const unsigned int semilla = base ^ (0x9e3779b9U * static_cast<unsigned int>(rank + 1));
            std::srand(semilla);
            omp_set_dynamic(0);
            if (std::getenv("OMP_NUM_THREADS") == nullptr) omp_set_num_threads(2);
            Contexto ctx{equipo, rank, cantidad, inicio, c.n, detallado, semilla, *registro};
            Comunicacion comunicacion(rank, procesos, bloque, c.version);
            OperacionesArreglos operaciones;
            Arreglos local, global;
            bool creados = false, llenos = false;
            registro->evento("Inicio", fecha() + ", version=" + nombreVersion(c.version)
                + ", N=" + std::to_string(c.n) + ", procesos=" + std::to_string(procesos)
                + ", hilos=" + std::to_string(omp_get_max_threads())
                + ", semilla=" + std::to_string(semilla), -1, -1, detallado);
            if (rank == 0) registro->escribir("N=" + std::to_string(c.n) + "; version=" + nombreVersion(c.version));
            registro->mostrarPendientes();
            while (true) {
                int opcion = 12, colectiva = c.colectiva;
                if (rank == 0) {
                    menu(*registro);
                    opcion = leerEntero(*registro, "Selecciona una opcion (1-12):", 1, 12, 12);
                    if (creados && llenos && opcion >= 8 && opcion <= 11 && c.version == Version::Reduce && colectiva == 0)
                        colectiva = leerEntero(*registro, "Variante: 1. MPI_Reduce / 2. MPI_Allreduce", 1, 2, 0);
                    if (creados && llenos && colectiva == 0 && opcion >= 8 && opcion <= 11 && c.version == Version::Reduce) opcion = 12;
                }
                MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);
                MPI_Bcast(&colectiva, 1, MPI_INT, 0, MPI_COMM_WORLD);
                if (opcion == 12) break;
                if (opcion != 1 && !creados) {
                    if (rank == 0) registro->escribir("Primero selecciona 1. Crear arreglos.");
                    continue;
                }
                if (opcion != 1 && opcion != 6 && opcion != 7 && !llenos) {
                    if (rank == 0) registro->escribir("Primero llena A y B con la opcion 6 o 7.");
                    continue;
                }
                MPI_Barrier(MPI_COMM_WORLD);
                const double comienzo = MPI_Wtime();
                Entero resultado = 0;
                double promedio = 0;
                if (opcion == 1) {
                    operaciones.crearArregloMPI(local.A, local.B, local.C, bloque, ctx);
                    if (rank == 0) global.crear(static_cast<std::size_t>(bloque) * procesos);
                    creados = true; llenos = false;
                } else if (opcion == 6 || opcion == 7) {
                    if (rank != 0) {
                        if (opcion == 6) operaciones.llenarSecuencial(local.A, local.B, ctx);
                        else operaciones.llenarAleatorio(local.A, local.B, ctx);
                    }
                    comunicacion.reunir(local.A, global.A, 11);
                    comunicacion.reunir(local.B, global.B, 12);
                    llenos = true;
                } else {
                    comunicacion.distribuir(global.A, local.A, 21);
                    if (opcion >= 2 && opcion <= 4) comunicacion.distribuir(global.B, local.B, 22);
                    if (opcion >= 2 && opcion <= 5) {
                        if (rank != 0) {
                            if (opcion == 2) operaciones.sumarArreglosOpenMPI(local.A, local.B, local.C, ctx);
                            if (opcion == 3) operaciones.restarArreglosOpenMPI(local.A, local.B, local.C, ctx);
                            if (opcion == 4) operaciones.multiplicarArreglosOpenMPI(local.A, local.B, local.C, ctx);
                            if (opcion == 5) operaciones.cuadradoArregloOpenMPI(local.A, local.C, ctx);
                        }
                        comunicacion.reunir(local.C, global.C, 23);
                    } else {
                        const bool usarColectivas = c.version == Version::Reduce;
                        const bool todos = colectiva == 2;
                        // La clase realiza la reduccion OpenMP local y la comunicacion global.
                        if (opcion == 8) resultado = operaciones.sumatoriaMPI(local.A, ctx, comunicacion, usarColectivas, todos);
                        if (opcion == 9) promedio = operaciones.promedioMPI(local.A, ctx, comunicacion, usarColectivas, todos);
                        if (opcion == 10) resultado = operaciones.maximoMPI(local.A, ctx, comunicacion, usarColectivas, todos);
                        if (opcion == 11) resultado = operaciones.minimoMPI(local.A, ctx, comunicacion, usarColectivas, todos);
                    }
                }
                if (detallado) registro->mostrarPendientes();
                MPI_Barrier(MPI_COMM_WORLD);
                const double tiempoLocal = MPI_Wtime() - comienzo;
                double tiempoGlobal = 0;
                MPI_Allreduce(&tiempoLocal, &tiempoGlobal, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
                const std::string metodo = opcion >= 8 && c.version == Version::Reduce
                                         ? (colectiva == 2 ? "MPI_Allreduce" : "MPI_Reduce") : "base";
                registro->evento("Tiempo local " + std::string(nombreOperacion(opcion)), decimal(tiempoLocal) + " s", -1, -1, false);
                registro->evento("Tiempo global " + std::string(nombreOperacion(opcion)), decimal(tiempoGlobal) + " s; "
                                 + nombreVersion(c.version) + "; " + metodo, -1, -1, rank == 0);
                if (opcion >= 8 && (rank == 0 || (c.version == Version::Reduce && colectiva == 2))) {
                    const std::string valor = opcion == 9 ? decimal(promedio) : std::to_string(resultado);
                    registro->evento("Resultado " + std::string(nombreOperacion(opcion)) + " " + metodo,
                                     valor, -1, -1, detallado);
                }
                if (rank == 0) {
                    if (detallado && opcion >= 2 && opcion <= 5) {
                        for (int i = 0; i < c.n; ++i)
                            registro->evento("Resultado reunido " + std::string(nombreOperacion(opcion)),
                                             std::to_string(global.C[bloque + i]), -1, i);
                    }
                }
                if (detallado) registro->mostrarPendientes();
                registro->comprobar();
            }
            // Arreglos se liberan aqui, antes del cierre de logs y MPI_Finalize.
        }
        // El maestro imprime el cierre al final; cada log termina con el equipo.
        for (int p = procesos - 1; p >= 0; --p) {
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == p) registro->escribir(integrantes);
            registro->mostrarPendientes();
        }
        registro->cerrar();
        delete registro; registro = nullptr;
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        return estado;
    } catch (const std::exception& error) {
        if (registro) {
            registro->evento("ERROR", error.what());
            registro->vaciarConsolaLocal();
        }
        else std::cerr << "[Equipo: " << equipo << "] [Proceso MPI: " << rank << "] ERROR: " << error.what() << '\n';
        delete registro;
        // Un error fatal se comunica a todos; evita trabajadores bloqueados.
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
}
