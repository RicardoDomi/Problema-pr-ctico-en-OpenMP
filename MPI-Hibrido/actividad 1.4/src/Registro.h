#pragma once

#include "Mensajes.h"

#include <fstream>
#include <mutex>
#include <string>

class Registro {
    std::ofstream archivo_;
    std::mutex cerrojo_;
    std::string computadora_;
    std::string pendientesConsola_;
    int rank_;

    void imprimir(const char* texto, std::size_t longitud);

public:
    Registro(const std::string& directorio, const std::string& computadora, int rank);
    ~Registro();

    Registro(const Registro&) = delete;
    Registro& operator=(const Registro&) = delete;

    void lineaEspecial(const std::string& texto, bool consola);
    void escribir(const std::string& texto, bool consola = false);
    void transaccion(const Transaccion& solicitud,
                     const RespuestaServidor& respuesta,
                     const std::string& contexto,
                     bool consola);
    void mostrarPendientesEnServidor();
    void cerrar();
};
