#include "ClienteBancario.h"
#include "CuentaBancaria.h"
#include "Mensajes.h"
#include "Registro.h"
#include "ServidorBancario.h"

#include <mpi.h>
#include <omp.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
const char* INTEGRANTES =
    "Castolo Gonzales Jorge Natanael | Dominguez Alcala Ricardo | "
    "Silva Montes Diego Eduardo";

int detectarHilos() {
    int hilos = 1;
#pragma omp parallel
    {
#pragma omp master
        hilos = omp_get_num_threads();
    }
    return hilos;
}

int leerOpcion(Registro& registro) {
    registro.escribir("1. Iniciar Simulacion Basica (5 operaciones por cliente)", true);
    registro.escribir("2. Iniciar Simulacion Masiva (100000 operaciones por cliente)", true);
    registro.escribir("3. Consultar Saldo Final del Servidor", true);
    registro.escribir("4. Salir", true);

    while (true) {
        registro.escribir("Selecciona una opcion (1-4):", true);
        int opcion = 0;
        if (std::cin >> opcion && opcion >= 1 && opcion <= 4) return opcion;
        if (std::cin.eof()) return 4;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        registro.escribir("Entrada invalida; ingresa un entero entre 1 y 4.", true);
    }
}

std::string dinero(double valor) {
    std::ostringstream salida;
    salida << std::fixed << std::setprecision(2) << '$' << valor;
    return salida.str();
}

std::string segundos(double valor) {
    std::ostringstream salida;
    salida << std::fixed << std::setprecision(6) << valor << " segundos";
    return salida.str();
}
}

int main(int argc, char** argv) {
    int soporteHilos = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &soporteHilos);

    int rank = 0;
    int procesos = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);

    char nombreProcesador[MPI_MAX_PROCESSOR_NAME]{};
    int longitudNombre = 0;
    MPI_Get_processor_name(nombreProcesador, &longitudNombre);
    const std::string computadora(nombreProcesador, longitudNombre);

    std::string directorioLogs = "logs";
    for (int indice = 1; indice < argc; ++indice) {
        const std::string argumento = argv[indice];
        if (argumento == "--logs" && indice + 1 < argc) {
            directorioLogs = argv[++indice];
        } else {
            if (rank == 0) std::cerr << "Argumento no reconocido: " << argumento << '\n';
            MPI_Finalize();
            return 1;
        }
    }

    try {
        Registro registro(directorioLogs, computadora, rank);
        registro.lineaEspecial(INTEGRANTES, rank == 0);

        if (soporteHilos < MPI_THREAD_FUNNELED) {
            if (rank == 0) registro.escribir("MPI no proporciona MPI_THREAD_FUNNELED", true);
            registro.lineaEspecial(INTEGRANTES, false);
            registro.cerrar();
            MPI_Finalize();
            return 1;
        }

        if (procesos != 5) {
            if (rank == 0) {
                registro.escribir("Esta actividad requiere exactamente 5 procesos MPI: "
                                  "1 servidor y 4 clientes.", true);
            }
            registro.lineaEspecial(INTEGRANTES, false);
            if (rank == 0) std::cout << INTEGRANTES << '\n';
            registro.cerrar();
            MPI_Finalize();
            return 1;
        }

        const int hilos = detectarHilos();
        registro.escribir("Inicio del nodo [Hilos OpenMP: " +
                          std::to_string(hilos) + "]", false);

        TiposMPI tiposMPI;
        CuentaBancaria cuenta(10000.0);
        ServidorBancario servidor(cuenta, registro, tiposMPI);
        ClienteBancario cliente(rank, registro, tiposMPI);

        int opcion = 0;
        while (opcion != 4) {
            if (rank == 0) opcion = leerOpcion(registro);
            MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

            if (opcion == 1 || opcion == 2) {
                const int operacionesPorCliente = opcion == 1 ? 5 : 100000;
                const bool detallada = opcion == 1;
                MPI_Barrier(MPI_COMM_WORLD);
                const double inicio = MPI_Wtime();

                if (rank == 0) servidor.ejecutar(operacionesPorCliente, procesos);
                else cliente.ejecutar(operacionesPorCliente, detallada);

                MPI_Barrier(MPI_COMM_WORLD);
                const double tiempoLocal = MPI_Wtime() - inicio;
                double tiempoGlobal = 0.0;
                MPI_Reduce(&tiempoLocal, &tiempoGlobal, 1, MPI_DOUBLE,
                           MPI_MAX, 0, MPI_COMM_WORLD);
                MPI_Bcast(&tiempoGlobal, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

                registro.mostrarPendientesEnServidor();
                registro.escribir("Tiempo local: " + segundos(tiempoLocal) +
                                  " | Tiempo global: " + segundos(tiempoGlobal),
                                  false);

                if (rank == 0) {
                    registro.escribir("Tiempo total: " + segundos(tiempoGlobal), true);
                    registro.escribir("Transacciones procesadas: " +
                                      std::to_string(servidor.procesadas()), true);
                    registro.escribir("Saldo final consolidado: " +
                                      dinero(cuenta.saldo()), true);
                }
            } else if (opcion == 3) {
                double saldoServidor = rank == 0 ? cuenta.saldo() : 0.0;
                MPI_Bcast(&saldoServidor, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
                registro.escribir("Consulta de saldo final del servidor: " +
                                  dinero(saldoServidor), rank == 0);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        registro.lineaEspecial(INTEGRANTES, false);
        registro.cerrar();
        if (rank == 0) std::cout << INTEGRANTES << '\n';
    } catch (const std::exception& error) {
        std::cerr << "[Proceso MPI: " << rank << "] Error: " << error.what() << '\n';
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Finalize();
    return 0;
}
