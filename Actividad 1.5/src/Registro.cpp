#include "Registro.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
std::string nombreSeguro(std::string texto) {
    for (char& c : texto) {
        const bool valido = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!valido) c = '_';
    }
    return texto;
}
}

Registro::Registro(const std::string& directorio, const std::string& equipo, int rank)
    : equipo_(equipo), rank_(rank) {
    std::filesystem::create_directories(directorio);
    ruta_ = directorio + "/log_equipo_" + nombreSeguro(equipo_) +
            "_nodo_" + std::to_string(rank_) + ".txt";
    archivo_.open(ruta_, std::ios::app);
    if (!archivo_) throw std::runtime_error("No se pudo abrir el log: " + ruta_);
}

Registro::~Registro() {
    archivo_.close();
}

std::string Registro::prefijo(int hilo, long long inicio, long long fin) const {
    std::ostringstream salida;
    salida << "[Equipo: " << equipo_ << "] [Nodo MPI: " << rank_
           << "] [Hilo OpenMP: " << hilo << "] [Bloque: ";
    if (inicio < 0) salida << "N/A";
    else if (fin < inicio) salida << "vacio";
    else salida << inicio << " - " << fin;
    salida << "] ";
    return salida.str();
}

void Registro::escribir(const std::string& linea, bool consola) {
    #pragma omp critical(registro_actividad_15)
    {
        archivo_ << linea << '\n';
        archivo_.flush();
        if (consola) {
            std::cout << linea << '\n';
            std::cout.flush();
        }
    }
}

void Registro::general(const std::string& mensaje, bool consola) {
    escribir(prefijo(0, -1, -1) + mensaje, consola);
}

void Registro::busqueda(int hilo, long long inicio, long long fin,
                        const std::string& algoritmo, long long resultado,
                        bool consola) {
    std::ostringstream salida;
    salida << prefijo(hilo, inicio, fin) << "[Algoritmo: " << algoritmo << "] [Estado: ";
    if (resultado >= 0) salida << "Elemento encontrado en indice global: " << resultado;
    else salida << "Elemento no encontrado";
    salida << ']';
    escribir(salida.str(), consola);
}

const std::string& Registro::ruta() const { return ruta_; }
