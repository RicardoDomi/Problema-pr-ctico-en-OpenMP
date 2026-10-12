#include "FuerzaBruta.h"

#include "Registro.h"

#include <omp.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <thread>

namespace {
enum EtiquetaMPI { RANGO = 10, HALLAZGO = 11, PARADA = 12, COMPLETADO = 13 };
constexpr Indice BLOQUES_POR_HILO = 64;
constexpr Indice INTERVALO_PARADA = 4096;
}

FuerzaBruta::FuerzaBruta() : alfabeto_(new char[37]), cantidadCaracteres_(36) {
    const std::string simbolos = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::copy(simbolos.begin(), simbolos.end(), alfabeto_);
    alfabeto_[cantidadCaracteres_] = '\0';
}

FuerzaBruta::~FuerzaBruta() { delete[] alfabeto_; }

bool FuerzaBruta::validarClave(const std::string& clave, std::string& error) const {
    if (clave.empty() || clave.size() > 12) {
        error = "La clave debe contener entre 1 y 12 caracteres.";
        return false;
    }
    for (char caracter : clave) {
        if (std::strchr(alfabeto_, caracter) == nullptr) {
            error = "Solo se permiten letras A-Z y digitos 0-9.";
            return false;
        }
    }
    error.clear();
    return true;
}

Indice FuerzaBruta::calcularTotal(int longitud) const {
    Indice total = 1;
    for (int i = 0; i < longitud; ++i) {
        if (total > std::numeric_limits<Indice>::max() /
                        static_cast<Indice>(cantidadCaracteres_)) {
            return 0;
        }
        total *= static_cast<Indice>(cantidadCaracteres_);
    }
    return total;
}

void FuerzaBruta::convertirIndice(Indice indice, char* destino, int longitud) const {
    for (int posicion = longitud - 1; posicion >= 0; --posicion) {
        destino[posicion] = alfabeto_[indice % cantidadCaracteres_];
        indice /= cantidadCaracteres_;
    }
    destino[longitud] = '\0';
}

ResultadoBusqueda FuerzaBruta::ejecutarSecuencial(const std::string& clave,
                                                   Registro& registro) const {
    ResultadoBusqueda resultado;
    const Indice total = calcularTotal(static_cast<int>(clave.size()));
    auto candidato = std::make_unique<char[]>(clave.size() + 1);
    const double inicio = MPI_Wtime();

    registro.escribir("[SECUENCIAL INICIO] Un nodo y un hilo. Espacio: " +
                      std::to_string(total));
    for (Indice indice = 0; indice < total; ++indice) {
        convertirIndice(indice, candidato.get(), static_cast<int>(clave.size()));
        ++resultado.revisadas;
        if (clave.size() == 3) {
            registro.escribir("[Hilo OpenMP: 0] [Candidato: " +
                              std::string(candidato.get()) + "] [Estado: Evaluando]");
        }
        if (clave == candidato.get()) {
            resultado.encontrada = true;
            resultado.indiceGanador = indice;
            break;
        }
    }

    resultado.segundos = MPI_Wtime() - inicio;
    resultado.rankGanador = 0;
    resultado.hiloGanador = 0;
    registro.escribir("[SECUENCIAL FIN] Clave: " + clave +
                      " | Revisadas: " + std::to_string(resultado.revisadas) +
                      " | Tiempo: " + std::to_string(resultado.segundos) + " s");
    return resultado;
}

ResultadoBusqueda FuerzaBruta::ejecutarTrabajador(const std::string& clave,
                                                   int rank,
                                                   const std::string& equipo,
                                                   Registro& registro) const {
    auto rango = std::make_unique<Indice[]>(2);
    MPI_Recv(rango.get(), 2, MPI_UNSIGNED_LONG_LONG, 0, RANGO,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    const Indice inicioRango = rango[0];
    const Indice finRango = rango[1];
    const Indice cantidad = finRango - inicioRango;
    registro.escribir("[PROCESO INICIO] Equipo: " + equipo +
                      " | Rango asignado: [" +
                      std::to_string(inicioRango) + ", " +
                      (cantidad ? std::to_string(finRango - 1) : std::string("vacio")) + "]");

    int senalParada = 0;
    MPI_Request solicitudParada = MPI_REQUEST_NULL;
    MPI_Irecv(&senalParada, 1, MPI_INT, 0, PARADA,
              MPI_COMM_WORLD, &solicitudParada);

    const int maximoHilos = std::max(1, omp_get_max_threads());
    const std::size_t longitudBuffer = clave.size() + 1;
    auto buffers = std::make_unique<char[]>(
        static_cast<std::size_t>(maximoHilos) * longitudBuffer);
    auto datosHallazgo = std::make_unique<Indice[]>(3);
    std::atomic<bool> detenido{false};
    std::atomic<bool> encontradoLocal{false};
    std::atomic<Indice> revisadas{0};
    const Indice bloques = std::min<Indice>(
        cantidad, static_cast<Indice>(maximoHilos) * BLOQUES_POR_HILO);
    const Indice pasoProgreso = std::max<Indice>(1, cantidad / 10);
    std::atomic<Indice> siguienteProgreso{pasoProgreso};
    const double tiempoInicial = MPI_Wtime();
    ResultadoBusqueda resultado;

#pragma omp parallel shared(solicitudParada, datosHallazgo, buffers, resultado)
    {
        const int hilo = omp_get_thread_num();
        char* candidato = buffers.get() +
                          static_cast<std::size_t>(hilo) * longitudBuffer;

#pragma omp single
        registro.escribir("[ENCABEZADO] Hilos OpenMP activos: " +
                          std::to_string(omp_get_num_threads()));
        registro.escribir("[HILO INICIO] Hilo OpenMP: " + std::to_string(hilo));

#pragma omp for schedule(dynamic, 1)
        for (Indice bloque = 0; bloque < bloques; ++bloque) {
            if (detenido.load(std::memory_order_relaxed)) continue;

            const Indice base = cantidad / bloques;
            const Indice sobrantes = cantidad % bloques;
            const Indice inicioBloque = inicioRango + bloque * base +
                                        std::min(bloque, sobrantes);
            const Indice cantidadBloque = base + (bloque < sobrantes ? 1 : 0);
            const Indice finBloque = inicioBloque + cantidadBloque;
            Indice revisadasBloque = 0;

            for (Indice indice = inicioBloque; indice < finBloque; ++indice) {
                if (detenido.load(std::memory_order_relaxed)) break;

                if (revisadasBloque % INTERVALO_PARADA == 0) {
#pragma omp critical(llamadas_mpi)
                    {
                        int recibida = 0;
                        MPI_Test(&solicitudParada, &recibida, MPI_STATUS_IGNORE);
                        if (recibida) detenido.store(true);
                    }
                }
                if (detenido.load(std::memory_order_relaxed)) break;

                convertirIndice(indice, candidato, static_cast<int>(clave.size()));
                ++revisadasBloque;
                if (clave.size() == 3) {
                    registro.escribir("[Hilo OpenMP: " + std::to_string(hilo) +
                                      "] [Candidato: " + candidato +
                                      "] [Estado: Evaluando]");
                }

                if (clave == candidato) {
                    bool esperado = false;
                    if (encontradoLocal.compare_exchange_strong(esperado, true)) {
                        detenido.store(true);
                        resultado.encontrada = true;
                        resultado.rankGanador = rank;
                        resultado.hiloGanador = hilo;
                        resultado.indiceGanador = indice;
                        datosHallazgo[0] = static_cast<Indice>(rank);
                        datosHallazgo[1] = static_cast<Indice>(hilo);
                        datosHallazgo[2] = indice;
                        registro.escribir("[EXITO] [Hilo OpenMP: " +
                                          std::to_string(hilo) +
                                          "] [Clave encontrada: \"" + candidato +
                                          "\"] [Tiempo parcial: " +
                                          std::to_string(MPI_Wtime() - tiempoInicial) + " s]");
#pragma omp critical(llamadas_mpi)
                        MPI_Ssend(datosHallazgo.get(), 3, MPI_UNSIGNED_LONG_LONG,
                                  0, HALLAZGO, MPI_COMM_WORLD);
                    }
                }
            }

            const Indice acumuladas = revisadas.fetch_add(revisadasBloque) +
                                       revisadasBloque;
            if (clave.size() != 3 && revisadasBloque > 0) {
                Indice marca = siguienteProgreso.load();
                if (acumuladas >= marca &&
                    siguienteProgreso.compare_exchange_strong(
                        marca, marca + pasoProgreso)) {
                    const double porcentaje = cantidad == 0
                        ? 100.0
                        : 100.0 * static_cast<double>(acumuladas) /
                                      static_cast<double>(cantidad);
                    registro.escribir("[PROGRESO] " +
                                      std::to_string(std::min(100.0, porcentaje)) +
                                      "% | Revisadas: " + std::to_string(acumuladas),
                                      false);
                }
            }
        }

        registro.escribir("[HILO FIN] Hilo OpenMP: " + std::to_string(hilo));
    }

    resultado.revisadas = revisadas.load();
    resultado.segundos = MPI_Wtime() - tiempoInicial;
    MPI_Send(&rank, 1, MPI_INT, 0, COMPLETADO, MPI_COMM_WORLD);
    MPI_Wait(&solicitudParada, MPI_STATUS_IGNORE);
    registro.escribir("[PROCESO FIN] Revisadas: " +
                      std::to_string(resultado.revisadas) +
                      " | Finalizacion limpia confirmada.");
    return resultado;
}

ResultadoBusqueda FuerzaBruta::ejecutarMaestro(const std::string& clave,
                                                Indice total,
                                                int procesos,
                                                Registro& registro) const {
    const int trabajadores = procesos - 1;
    auto rango = std::make_unique<Indice[]>(2);
    Indice cursor = 0;
    const double inicio = MPI_Wtime();

    registro.escribir("[MAESTRO INICIO] Espacio 36^" +
                      std::to_string(clave.size()) + " = " +
                      std::to_string(total));
    for (int rank = 1; rank < procesos; ++rank) {
        const Indice cantidad = total / trabajadores +
            (static_cast<Indice>(rank - 1) < total % trabajadores ? 1 : 0);
        rango[0] = cursor;
        rango[1] = cursor + cantidad;
        cursor = rango[1];
        registro.escribir("[RANGO] Nodo " + std::to_string(rank) + " recibe [" +
                          std::to_string(rango[0]) + ", " +
                          (cantidad ? std::to_string(rango[1] - 1)
                                    : std::string("vacio")) + "]");
        MPI_Send(rango.get(), 2, MPI_UNSIGNED_LONG_LONG, rank, RANGO,
                 MPI_COMM_WORLD);
    }

    auto solicitudes = std::make_unique<MPI_Request[]>(trabajadores);
    for (int i = 0; i < trabajadores; ++i) solicitudes[i] = MPI_REQUEST_NULL;
    auto hallazgo = std::make_unique<Indice[]>(3);
    int completados = 0;
    int senalParada = 1;
    bool paradaEnviada = false;
    ResultadoBusqueda resultado;

    while (completados < trabajadores) {
        int disponible = 0;
        MPI_Status estado{};
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                   &disponible, &estado);
        if (!disponible) {
            std::this_thread::yield();
            continue;
        }

        if (estado.MPI_TAG == HALLAZGO) {
            MPI_Recv(hallazgo.get(), 3, MPI_UNSIGNED_LONG_LONG,
                     estado.MPI_SOURCE, HALLAZGO, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            if (!resultado.encontrada) {
                resultado.encontrada = true;
                resultado.segundos = MPI_Wtime() - inicio;
                resultado.rankGanador = static_cast<int>(hallazgo[0]);
                resultado.hiloGanador = static_cast<int>(hallazgo[1]);
                resultado.indiceGanador = hallazgo[2];
                registro.escribir("[HALLAZGO SINCRONO] Nodo " +
                                  std::to_string(resultado.rankGanador) +
                                  " | Hilo " + std::to_string(resultado.hiloGanador) +
                                  " | Clave: " + clave);
            }
        } else if (estado.MPI_TAG == COMPLETADO) {
            int rankFinalizado = -1;
            MPI_Recv(&rankFinalizado, 1, MPI_INT, estado.MPI_SOURCE,
                     COMPLETADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++completados;
            registro.escribir("[COMPLETADO] Nodo " +
                              std::to_string(rankFinalizado) +
                              " termino (" + std::to_string(completados) + "/" +
                              std::to_string(trabajadores) + ").");
        }

        if (!paradaEnviada && (resultado.encontrada || completados == trabajadores)) {
            paradaEnviada = true;
            for (int rank = 1; rank < procesos; ++rank) {
                MPI_Isend(&senalParada, 1, MPI_INT, rank, PARADA,
                          MPI_COMM_WORLD, &solicitudes[rank - 1]);
            }
            registro.escribir("[PARADA GLOBAL] Orden enviada a todos los trabajadores.");
        }
    }

    MPI_Waitall(trabajadores, solicitudes.get(), MPI_STATUSES_IGNORE);
    if (!resultado.encontrada) resultado.segundos = MPI_Wtime() - inicio;
    registro.escribir("[MAESTRO FIN] Todos los procesos finalizaron limpiamente.");
    return resultado;
}
