#include <mpi.h>
#include <omp.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <cstring>

double tiempoSecuencial = 0.0;
double tiempoParalelo = 0.0;

void convertirIndiceATexto(unsigned long long indice, char* candidato, int longitud) {
    static constexpr char alfabeto[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int posicion = longitud - 1; posicion >= 0; --posicion) {
        candidato[posicion] = alfabeto[indice % 36];
        indice /= 36;
    }
    candidato[longitud] = '\0';
}

void ejecutarBusquedaSecuencial(const std::string& claveObjetivo, std::ofstream& log) {
    unsigned long long totalCombinaciones = 1;
    for (std::size_t i = 0; i < claveObjetivo.length(); ++i) {
        totalCombinaciones *= 36;
    }

    omp_set_dynamic(0);
    omp_set_num_threads(1);
    double inicio = MPI_Wtime();
    char candidato[37];
    unsigned long long indiceEncontrado = totalCombinaciones;

    for (unsigned long long indice = 0; indice < totalCombinaciones; ++indice) {
        convertirIndiceATexto(indice, candidato, claveObjetivo.length());
        if (claveObjetivo == candidato) {
            indiceEncontrado = indice;
            break;
        }
    }

    double tiempo = MPI_Wtime() - inicio;
    if (indiceEncontrado < totalCombinaciones) {
        tiempoSecuencial = tiempo;
        std::cout << "Clave encontrada secuencialmente en " << tiempo << " segundos.\n";
        log << std::fixed << std::setprecision(4)
            << "[BASELINE] Clave encontrada: \"" << candidato << "\" | Indice: "
            << indiceEncontrado << " | Tiempo: " << tiempo << "s\n";
    }
}

void mostrarComparativa() {
    std::cout << "\n--- Comparativa de tiempos ---\n"
              << std::left << std::setw(20) << "Modo" << std::right << std::setw(16) << "Tiempo (s)\n"
              << std::left << std::setw(20) << "Secuencial" << std::right << std::setw(16)
              << std::fixed << std::setprecision(6) << tiempoSecuencial << '\n'
              << std::left << std::setw(20) << "MPI + OpenMP" << std::right << std::setw(16)
              << tiempoParalelo << '\n';

    if (tiempoSecuencial > 0.0 && tiempoParalelo > 0.0) {
        std::cout << "Speedup: " << tiempoSecuencial / tiempoParalelo << "x\n";
    } else {
        std::cout << "Speedup no disponible: ejecute ambos modos primero.\n";
    }
}

void ejecutarEsclavo(unsigned long long inicio, unsigned long long fin, int longitud,
                     const std::string& claveObjetivo, int rank, const char* equipo,
                     std::ofstream& log) {
    log << "Esclavo recibio rango numerico asignado: [" << inicio << ", " << fin << "]\n";
    bool encontrado = false;

#pragma omp parallel for schedule(dynamic) shared(encontrado)
    for (unsigned long long idx = inicio; idx < fin; ++idx) {
        if (encontrado) continue;

        char candidato[37];
        convertirIndiceATexto(idx, candidato, longitud);
        int tid = omp_get_thread_num();

        if (std::strcmp(candidato, claveObjetivo.c_str()) == 0) {
#pragma omp critical
            {
                if (!encontrado) {
                    encontrado = true;
                    int exito = 1;
                    log << "[EXITO] [Equipo: " << equipo
                        << "] [Nodo MPI: " << rank << "] [Hilo OpenMP: " << tid
                        << "] [Clave Encontrada: \"" << candidato << "\"]"
                        << " [Indice: " << idx << "]\n";
                    log.flush();
                    MPI_Send(&exito, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rango;
    int cantidadProcesos;
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    MPI_Comm_size(MPI_COMM_WORLD, &cantidadProcesos);

    char nombreEquipo[MPI_MAX_PROCESSOR_NAME];
    int longitudNombreEquipo;
    MPI_Get_processor_name(nombreEquipo, &longitudNombreEquipo);
    std::ofstream log("log_equipo_" + std::string(nombreEquipo) + "_nodo_" + std::to_string(rango) + ".txt");
    if (!log) {
        std::cerr << "No se pudo abrir el archivo de log del nodo " << rango << ".\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    log << "[Encabezado] Equipo: " << nombreEquipo << " | Rango MPI: " << rango
        << " | Hilos OpenMP activos: " << omp_get_max_threads() << '\n'
        << "[INICIO] Proceso iniciado. Rango MPI asignado: " << rango << '\n';
    log.flush();

    int opcion = 0;
    std::string target;
    const char caracteresValidos[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int longitudClave = 0;

    do {
        if (rango == 0) {
            std::cout << "\n--- Menu ---\n"
                      << "1. Ingresar clave personalizada en mayusculas (A-Z, 0-9).\n"
                      << "2. Ejecutar busqueda en modo Secuencial (1 nodo, 1 hilo - Medicion de Linea Base).\n"
                      << "3. Ejecutar busqueda en modo Paralelo Distribuido (MPI + OpenMP).\n"
                      << "4. Mostrar tabla comparativa de tiempos y ganancia de velocidad (Speedup).\n"
                      << "5. Salir.\n"
                      << "Seleccione una opcion:\n" << std::flush;
            std::cin >> opcion;
        }

        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (opcion == 1) {
            if (rango == 0) {
                bool valida;
                do {
                    std::cout << "Ingrese la clave (A-Z, 0-9; maximo 36 caracteres):\n" << std::flush;
                    std::cin >> target;
                    longitudClave = target.length();
                    valida = longitudClave > 0 && longitudClave <= 36;

                    for (int i = 0; valida && i < longitudClave; ++i) {
                        valida = target[i] >= 'A' && target[i] <= 'Z' || target[i] >= '0' && target[i] <= '9';
                    }

                    if (!valida) {
                        std::cout << "Clave invalida. Use solo letras mayusculas A-Z y digitos 0-9.\n";
                    }
                } while (!valida);
            }

            MPI_Bcast(&longitudClave, 1, MPI_INT, 0, MPI_COMM_WORLD);
            target.resize(longitudClave);
            MPI_Bcast(target.data(), longitudClave, MPI_CHAR, 0, MPI_COMM_WORLD);
        }

        if (opcion == 2 && rango == 0) {
            if (longitudClave > 0) {
                ejecutarBusquedaSecuencial(target, log);
            } else {
                std::cout << "Primero ingrese una clave con la opcion 1.\n";
            }
        }

        if (opcion == 4 && rango == 0) {
            mostrarComparativa();
        }

        if (opcion == 3 && longitudClave > 0) {
            double inicioBusqueda = MPI_Wtime();
            if (cantidadProcesos < 2) {
                if (rango == 0) {
                    std::cerr << "La busqueda distribuida requiere al menos 2 procesos MPI.\n";
                }
                continue;
            }

            unsigned long long totalCombinaciones = 1;
            for (int i = 0; i < longitudClave; ++i) {
                totalCombinaciones *= 36;
            }

            if (rango == 0) {
                log << "Inicio de busqueda distribuida. Rango asignado: [0, "
                    << totalCombinaciones << "]\n";
                for (int proceso = 1; proceso < cantidadProcesos; ++proceso) {
                    unsigned long long inicio = totalCombinaciones * (proceso - 1) / (cantidadProcesos - 1);
                    unsigned long long fin = totalCombinaciones * proceso / (cantidadProcesos - 1);
                    unsigned long long limites[] = {inicio, fin};
                    log << "Rango enviado al nodo MPI " << proceso << ": [" << inicio << ", " << fin << "]\n";
                    MPI_Send(limites, 2, MPI_UNSIGNED_LONG_LONG, proceso, 0, MPI_COMM_WORLD);
                }

                int encontrada = 0;
                int rangoEncontrado = -1;
                while (!encontrada) {
                    int disponible;
                    MPI_Status estado;
                    MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &disponible, &estado);

                    if (disponible) {
                        int exito;
                        MPI_Recv(&exito, 1, MPI_INT, estado.MPI_SOURCE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (exito) {
                            encontrada = 1;
                            rangoEncontrado = estado.MPI_SOURCE;
                        }
                    }
                }

                double tiempoTotal = MPI_Wtime() - inicioBusqueda;
                tiempoParalelo = tiempoTotal;
                std::cout << "Clave descifrada por el proceso (rango) " << rangoEncontrado
                          << " en " << tiempoTotal << " segundos.\n";
                log << std::fixed << std::setprecision(4)
                    << "[EXITO] Rango MPI que reporto el hallazgo: " << rangoEncontrado
                    << " | Tiempo total: " << tiempoTotal << "s\n";
                int detener = 1;
                for (int proceso = 1; proceso < cantidadProcesos; ++proceso) {
                    MPI_Send(&detener, 1, MPI_INT, proceso, 2, MPI_COMM_WORLD);
                }
            } else {
                unsigned long long limites[2];
                MPI_Recv(limites, 2, MPI_UNSIGNED_LONG_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                log << "Inicio de busqueda distribuida. Rango asignado: [" << limites[0]
                    << ", " << limites[1] << "]\n";
                ejecutarEsclavo(limites[0], limites[1], longitudClave, target,
                                rango, nombreEquipo, log);
                int disponible = 0;
                while (!disponible) {
                    MPI_Iprobe(0, 2, MPI_COMM_WORLD, &disponible, MPI_STATUS_IGNORE);
                }
                int detener;
                MPI_Recv(&detener, 1, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                log << "[PARADA] Senal de parada recibida del maestro.\n";
            }
        }
    } while (opcion != 5);

    log << "[FIN] Proceso finalizado limpiamente. Rango MPI: " << rango << '\n';
    log.flush();
    log.close();
    MPI_Finalize();
    return 0;
}
