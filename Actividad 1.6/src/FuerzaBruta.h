#ifndef FUERZA_BRUTA_H
#define FUERZA_BRUTA_H

#include <mpi.h>

#include <string>

class Registro;

using Indice = unsigned long long;

struct ResultadoBusqueda {
    bool encontrada = false;
    double segundos = 0.0;
    Indice revisadas = 0;
    Indice indiceGanador = 0;
    int rankGanador = -1;
    int hiloGanador = -1;
};

class FuerzaBruta {
public:
    FuerzaBruta();
    ~FuerzaBruta();

    FuerzaBruta(const FuerzaBruta&) = delete;
    FuerzaBruta& operator=(const FuerzaBruta&) = delete;

    bool validarClave(const std::string& clave, std::string& error) const;
    Indice calcularTotal(int longitud) const;
    void convertirIndice(Indice indice, char* destino, int longitud) const;

    ResultadoBusqueda ejecutarSecuencial(const std::string& clave,
                                         Registro& registro) const;
    ResultadoBusqueda ejecutarTrabajador(const std::string& clave,
                                         int rank,
                                         const std::string& equipo,
                                         Registro& registro) const;
    ResultadoBusqueda ejecutarMaestro(const std::string& clave,
                                      Indice total,
                                      int procesos,
                                      Registro& registro) const;

private:
    char* alfabeto_;
    int cantidadCaracteres_;
};

#endif
