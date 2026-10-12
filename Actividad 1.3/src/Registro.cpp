#include "Registro.h"
#include <mpi.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

Registro::Registro(const std::string& directorio, const std::string& nombre, int proceso)
    : equipo(nombre), rank(proceso) {
    std::filesystem::create_directories(directorio);
    std::string seguro = nombre;
    for (char& c : seguro)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') c = '_';
    const std::string ruta = directorio + "/log_equipo_" + seguro + "_nodo_"
                           + std::to_string(rank) + ".txt";
    // Append conserva ejecuciones anteriores si se reutiliza el directorio.
    archivo.open(ruta, std::ios::app);
    if (!archivo) throw std::runtime_error("No se pudo abrir el log: " + ruta);
}

void Registro::escribir(const std::string& texto, bool consola) {
    std::lock_guard<std::mutex> bloqueo(cerrojo);
    const std::string linea = "[Equipo: " + equipo + "] [Proceso MPI: "
                            + std::to_string(rank) + "] " + texto + "\n";
    archivo << linea << std::flush;
    if (!archivo) fallo.store(true);
    if (consola && rank == 0) imprimir(linea.data(), linea.size());
    else if (consola) pendientes += linea;
}

void Registro::imprimir(const char* texto, std::size_t longitud) {
    if (std::fwrite(texto, 1, longitud, stdout) != longitud) fallo.store(true);
    if (std::fflush(stdout) != 0) fallo.store(true);
}

void Registro::mostrarPendientes() {
    int procesos = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);
    // Solo transporta mensajes de registro; los arreglos usan Comunicacion.
    const int bloque = 65536;
    char* buffer = rank == 0 ? new char[bloque] : nullptr;
    for (int origen = 1; origen < procesos; ++origen) {
        if (rank == origen) {
            const unsigned long long longitud = pendientes.size();
            MPI_Send(&longitud, 1, MPI_UNSIGNED_LONG_LONG, 0, 90, MPI_COMM_WORLD);
            unsigned long long enviados = 0;
            while (enviados < longitud) {
                const int cantidad = static_cast<int>(std::min<unsigned long long>(bloque, longitud - enviados));
                MPI_Send(pendientes.data() + enviados, cantidad, MPI_CHAR, 0, 91, MPI_COMM_WORLD);
                enviados += cantidad;
            }
            pendientes.clear();
        } else if (rank == 0) {
            unsigned long long longitud = 0;
            MPI_Recv(&longitud, 1, MPI_UNSIGNED_LONG_LONG, origen, 90, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            unsigned long long recibidos = 0;
            while (recibidos < longitud) {
                const int cantidad = static_cast<int>(std::min<unsigned long long>(bloque, longitud - recibidos));
                MPI_Recv(buffer, cantidad, MPI_CHAR, origen, 91, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // El origen ya escribio este mismo texto en su propio log local.
                imprimir(buffer, cantidad);
                recibidos += cantidad;
            }
        }
    }
    delete[] buffer;
    MPI_Barrier(MPI_COMM_WORLD);
}

void Registro::vaciarConsolaLocal() {
    imprimir(pendientes.data(), pendientes.size());
    pendientes.clear();
}

void Registro::evento(const std::string& operacion, const std::string& valor,
                      int hilo, int posicion, bool consola) {
    std::string texto;
    if (hilo >= 0) texto += "[Hilo OpenMP: " + std::to_string(hilo) + "] ";
    if (posicion >= 0) texto += "[Posicion: " + std::to_string(posicion) + "] ";
    escribir(texto + "[Operacion: " + operacion + "] [Valor: " + valor + "]", consola);
}

void Registro::comprobar() const {
    if (fallo.load()) throw std::runtime_error("Error al persistir el log o escribir en consola");
}

void Registro::cerrar() {
    comprobar();
    archivo.close();
    if (archivo.fail()) throw std::runtime_error("Error al cerrar el log");
}
