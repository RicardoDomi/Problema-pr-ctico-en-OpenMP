#include "Registro.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
std::string nombreSeguro(std::string texto) {
    for (char& caracter : texto) {
        if (!std::isalnum(static_cast<unsigned char>(caracter)) && caracter != '-') {
            caracter = '_';
        }
    }
    return texto;
}
}

Registro::Registro(const std::string& directorio, const std::string& equipo, int rank)
    : prefijo_("[Equipo: " + equipo + "] [Nodo MPI: " + std::to_string(rank) + "] ") {
    std::filesystem::create_directories(directorio);
    const std::string ruta = directorio + "/log_equipo_" + nombreSeguro(equipo) +
                             "_nodo_" + std::to_string(rank) + ".txt";
    archivo_.open(ruta, std::ios::out | std::ios::trunc);
    if (!archivo_) throw std::runtime_error("No se pudo crear el log: " + ruta);
}

void Registro::escribir(const std::string& mensaje, bool mostrarEnConsola) {
#pragma omp critical(registro_salida)
    {
        archivo_ << prefijo_ << mensaje << '\n';
        if (mostrarEnConsola) std::cout << prefijo_ << mensaje << std::endl;
    }
}
