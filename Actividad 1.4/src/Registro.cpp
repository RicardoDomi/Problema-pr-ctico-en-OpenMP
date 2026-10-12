#include "Registro.h"

#include <mpi.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
constexpr int TAG_LOG_LONGITUD = 7001;
constexpr int TAG_LOG_DATOS = 7002;
}

Registro::Registro(const std::string& directorio,
                   const std::string& computadora,
                   int rank)
    : computadora_(computadora), rank_(rank) {
    std::filesystem::create_directories(directorio);
    std::string nombreSeguro = computadora;
    for (char& caracter : nombreSeguro) {
        if (!std::isalnum(static_cast<unsigned char>(caracter)) &&
            caracter != '-' && caracter != '_') {
            caracter = '_';
        }
    }

    const std::string ruta = directorio + "/log_equipo_" + nombreSeguro +
                             "_nodo_" + std::to_string(rank_) + ".txt";
    archivo_.open(ruta, std::ios::app);
    if (!archivo_) throw std::runtime_error("No se pudo abrir el log: " + ruta);
}

Registro::~Registro() {
    if (archivo_.is_open()) archivo_.close();
}

void Registro::imprimir(const char* texto, std::size_t longitud) {
    if (longitud == 0) return;
    if (std::fwrite(texto, 1, longitud, stdout) != longitud ||
        std::fflush(stdout) != 0) {
        throw std::runtime_error("No se pudo escribir en la consola");
    }
}

void Registro::lineaEspecial(const std::string& texto, bool consola) {
    std::lock_guard<std::mutex> bloqueo(cerrojo_);
    const std::string linea = texto + "\n";
    archivo_ << linea;
    if (!archivo_) throw std::runtime_error("No se pudo escribir en el log");
    if (consola) imprimir(linea.data(), linea.size());
}

void Registro::escribir(const std::string& texto, bool consola) {
    std::lock_guard<std::mutex> bloqueo(cerrojo_);
    const std::string linea = "[Equipo: " + computadora_ + "] [Proceso MPI: " +
                              std::to_string(rank_) + "] " + texto + "\n";
    archivo_ << linea;
    if (!archivo_) throw std::runtime_error("No se pudo escribir en el log");

    if (consola && rank_ == 0) imprimir(linea.data(), linea.size());
    else if (consola) pendientesConsola_ += linea;
}

void Registro::transaccion(const Transaccion& solicitud,
                           const RespuestaServidor& respuesta,
                           const std::string& contexto,
                           bool consola) {
    std::ostringstream detalle;
    detalle << "[Cliente MPI: " << solicitud.id_cliente << "] "
            << "[Hilo OpenMP: " << solicitud.id_hilo_openmp << "] "
            << "[Operacion: " << nombreOperacion(solicitud.tipo_operacion) << "] "
            << std::fixed << std::setprecision(2)
            << "[Monto: $" << solicitud.monto << "] "
            << "[Estado: " << (respuesta.aprobada ? "Aprobada" : "Rechazada") << "] "
            << "[Saldo: $" << respuesta.saldo_resultante << "] "
            << "[Mensaje: " << respuesta.mensaje << "] "
            << "[Flujo: " << contexto << "]";
    escribir(detalle.str(), consola);
}

void Registro::mostrarPendientesEnServidor() {
    int procesos = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);
    constexpr int TAMANO_BLOQUE = 65536;
    char* buffer = rank_ == 0 ? new char[TAMANO_BLOQUE] : nullptr;

    for (int origen = 1; origen < procesos; ++origen) {
        if (rank_ == origen) {
            const unsigned long long longitud = pendientesConsola_.size();
            MPI_Send(&longitud, 1, MPI_UNSIGNED_LONG_LONG, 0,
                     TAG_LOG_LONGITUD, MPI_COMM_WORLD);
            unsigned long long enviados = 0;
            while (enviados < longitud) {
                const int cantidad = static_cast<int>(
                    std::min<unsigned long long>(TAMANO_BLOQUE, longitud - enviados));
                MPI_Send(pendientesConsola_.data() + enviados, cantidad, MPI_CHAR,
                         0, TAG_LOG_DATOS, MPI_COMM_WORLD);
                enviados += cantidad;
            }
            pendientesConsola_.clear();
        } else if (rank_ == 0) {
            unsigned long long longitud = 0;
            MPI_Recv(&longitud, 1, MPI_UNSIGNED_LONG_LONG, origen,
                     TAG_LOG_LONGITUD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            unsigned long long recibidos = 0;
            while (recibidos < longitud) {
                const int cantidad = static_cast<int>(
                    std::min<unsigned long long>(TAMANO_BLOQUE, longitud - recibidos));
                MPI_Recv(buffer, cantidad, MPI_CHAR, origen, TAG_LOG_DATOS,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                imprimir(buffer, cantidad);
                recibidos += cantidad;
            }
        }
    }

    delete[] buffer;
    MPI_Barrier(MPI_COMM_WORLD);
}

void Registro::cerrar() {
    std::lock_guard<std::mutex> bloqueo(cerrojo_);
    archivo_.flush();
    if (!archivo_) throw std::runtime_error("No se pudo guardar el log");
    archivo_.close();
    if (archivo_.fail()) throw std::runtime_error("No se pudo cerrar el log");
}
