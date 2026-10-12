#include "FuerzaBruta.h"
#include "Registro.h"

#include <mpi.h>
#include <omp.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
int leerOpcion(Registro& registro) {
    registro.escribir(
        "\n1. Ingresar clave personalizada (A-Z, 0-9)"
        "\n2. Ejecutar busqueda secuencial (1 nodo, 1 hilo)"
        "\n3. Ejecutar busqueda paralela distribuida (MPI + OpenMP)"
        "\n4. Mostrar comparativa, Speedup y Eficiencia"
        "\n5. Salir"
        "\nSelecciona una opcion (1-5):");
    std::string linea;
    std::getline(std::cin, linea);
    return linea.size() == 1 && linea[0] >= '1' && linea[0] <= '5'
        ? linea[0] - '0' : 0;
}

std::string carpetaLogs(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--logs") return argv[i + 1];
    }
    return "logs";
}

std::string tabla(double secuencial, double paralelo, int recursos) {
    std::ostringstream salida;
    salida << std::fixed << std::setprecision(6)
           << "Modo | Tiempo (s)\n"
           << "Secuencial | ";
    if (secuencial < 0) salida << "no ejecutado";
    else salida << secuencial;
    salida << "\nMPI + OpenMP | ";
    if (paralelo < 0) salida << "no ejecutado";
    else salida << paralelo;
    if (secuencial > 0 && paralelo > 0) {
        const double speedup = secuencial / paralelo;
        salida << "\nSpeedup: " << speedup
               << "\nEficiencia: " << speedup / recursos
               << " (p = " << recursos << " hilos de trabajadores)";
    } else {
        salida << "\nSpeedup y eficiencia no disponibles.";
    }
    return salida.str();
}
}

int main(int argc, char** argv) {
    int nivelMPI = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &nivelMPI);

    int rank = 0;
    int procesos = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);

    try {
        if (nivelMPI < MPI_THREAD_SERIALIZED) {
            throw std::runtime_error("MPI no ofrece MPI_THREAD_SERIALIZED.");
        }

        const int capacidadEquipo = MPI_MAX_PROCESSOR_NAME + 1;
        auto nombreEquipo = std::make_unique<char[]>(capacidadEquipo);
        int longitudEquipo = 0;
        MPI_Get_processor_name(nombreEquipo.get(), &longitudEquipo);
        nombreEquipo[longitudEquipo] = '\0';
        const std::string equipo(nombreEquipo.get());

        auto equipos = rank == 0
            ? std::make_unique<char[]>(static_cast<std::size_t>(procesos) * capacidadEquipo)
            : std::unique_ptr<char[]>();
        MPI_Gather(nombreEquipo.get(), capacidadEquipo, MPI_CHAR,
                   equipos.get(), capacidadEquipo, MPI_CHAR, 0, MPI_COMM_WORLD);

        Registro registro(carpetaLogs(argc, argv), equipo, rank);
        FuerzaBruta buscador;
        const std::string integrantes =
            "Castolo Gonzales Jorge Natanael | Dominguez Alcala Ricardo | "
            "Silva Montes Diego Eduardo";
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) registro.escribir(integrantes);
        MPI_Barrier(MPI_COMM_WORLD);
        registro.escribir("[INICIO] Proceso activo | Nivel MPI: " +
                          std::to_string(nivelMPI) +
                          " | Hilos configurados: " +
                          std::to_string(omp_get_max_threads()));

        std::string clave;
        Indice total = 0;
        double tiempoSecuencial = -1.0;
        double tiempoParalelo = -1.0;
        int recursosParalelos = 0;
        int opcion = 0;

        while (opcion != 5) {
            if (rank == 0) opcion = leerOpcion(registro);
            MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (opcion == 0) {
                if (rank == 0) registro.escribir("Opcion invalida.");
                continue;
            }
            if (opcion == 5) break;

            if (opcion == 1) {
                int longitud = 0;
                if (rank == 0) {
                    std::string error;
                    do {
                        registro.escribir("Ingresa la clave objetivo en mayusculas:");
                        std::getline(std::cin, clave);
                        if (!buscador.validarClave(clave, error)) registro.escribir(error);
                    } while (!error.empty());
                    longitud = static_cast<int>(clave.size());
                    total = buscador.calcularTotal(longitud);
                    tiempoSecuencial = tiempoParalelo = -1.0;
                    registro.escribir("Clave aceptada. Espacio de busqueda: 36^" +
                                      std::to_string(longitud) + " = " +
                                      std::to_string(total));
                }
                MPI_Bcast(&longitud, 1, MPI_INT, 0, MPI_COMM_WORLD);
                clave.resize(longitud);
                MPI_Bcast(clave.data(), longitud, MPI_CHAR, 0, MPI_COMM_WORLD);
                MPI_Bcast(&total, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
                continue;
            }

            if ((opcion == 2 || opcion == 3) && clave.empty()) {
                if (rank == 0) registro.escribir("Primero debes ingresar una clave.");
                continue;
            }

            if (opcion == 2) {
                if (rank == 0) {
                    const ResultadoBusqueda resultado =
                        buscador.ejecutarSecuencial(clave, registro);
                    tiempoSecuencial = resultado.segundos;
                }
                MPI_Bcast(&tiempoSecuencial, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            } else if (opcion == 3) {
                if (procesos < 2) {
                    if (rank == 0) registro.escribir(
                        "La busqueda distribuida requiere al menos 2 procesos MPI.");
                    continue;
                }

                const int hilosLocales = rank == 0 ? 0 : omp_get_max_threads();
                MPI_Reduce(&hilosLocales, &recursosParalelos, 1, MPI_INT,
                           MPI_SUM, 0, MPI_COMM_WORLD);
                MPI_Barrier(MPI_COMM_WORLD);
                ResultadoBusqueda resultado = rank == 0
                    ? buscador.ejecutarMaestro(clave, total, procesos, registro)
                    : buscador.ejecutarTrabajador(clave, rank, equipo, registro);

                Indice revisadasGlobales = 0;
                MPI_Reduce(&resultado.revisadas, &revisadasGlobales, 1,
                           MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
                MPI_Barrier(MPI_COMM_WORLD);

                if (rank == 0) {
                    tiempoParalelo = resultado.segundos;
                    const std::string ganador = resultado.rankGanador >= 0
                        ? std::string(equipos.get() +
                              resultado.rankGanador * capacidadEquipo)
                        : std::string("ninguno");
                    registro.escribir("[RESULTADO DISTRIBUIDO] Clave: " + clave +
                                      " | Equipo ganador: " + ganador +
                                      " | Nodo: " + std::to_string(resultado.rankGanador) +
                                      " | Hilo: " + std::to_string(resultado.hiloGanador) +
                                      " | Revisadas: " + std::to_string(revisadasGlobales) +
                                      " | Tiempo total: " +
                                      std::to_string(tiempoParalelo) + " s");
                }
                MPI_Bcast(&tiempoParalelo, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
            } else if (opcion == 4 && rank == 0) {
                registro.escribir(tabla(tiempoSecuencial, tiempoParalelo,
                                        std::max(1, recursosParalelos)));
            }
        }

        registro.escribir("[FIN] Proceso MPI finalizado limpiamente.", false);
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            for (int proceso = 0; proceso < procesos; ++proceso) {
                registro.escribir("[FIN GLOBAL] Nodo MPI " +
                                  std::to_string(proceso) +
                                  " confirmo su finalizacion.");
            }
            registro.escribir(integrantes);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Nodo MPI " << rank << "] " << error.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
}
