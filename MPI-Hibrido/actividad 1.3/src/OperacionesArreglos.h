#pragma once
#include "Registro.h"
#include <mpi.h>
#include <string>

using Entero = long long;
class Comunicacion;

// Contexto que identifica la seccion local y al equipo/proceso que la ejecuta.
struct Contexto {
    std::string equipo;
    int rank;
    int cantidad;
    int inicio;
    int total;
    bool detallado;
    unsigned int semilla;
    Registro& log;
};

class OperacionesArreglos {
public:
    void crearArregloMPI(Entero*& A, Entero*& B, Entero*& C, int bloque,
                         const Contexto& ctx) const;
    void llenarSecuencial(Entero* A, Entero* B, const Contexto& ctx) const;
    void llenarAleatorio(Entero* A, Entero* B, const Contexto& ctx) const;
    void sumarArreglosOpenMPI(const Entero* A, const Entero* B, Entero* C,
                             const Contexto& ctx) const;
    void restarArreglosOpenMPI(const Entero* A, const Entero* B, Entero* C,
                              const Contexto& ctx) const;
    void multiplicarArreglosOpenMPI(const Entero* A, const Entero* B, Entero* C,
                                   const Contexto& ctx) const;
    void cuadradoArregloOpenMPI(const Entero* A, Entero* C, const Contexto& ctx) const;
    // Opciones 8 a 11: reciben el arreglo y realizan ambos niveles de reduccion.
    // El resultado es valido en rank 0 o en todos si se usa MPI_Allreduce.
    Entero sumatoriaMPI(const Entero* A, const Contexto& ctx,
                       const Comunicacion& comunicacion, bool colectiva, bool todos) const;
    double promedioMPI(const Entero* A, const Contexto& ctx,
                       const Comunicacion& comunicacion, bool colectiva, bool todos) const;
    Entero maximoMPI(const Entero* A, const Contexto& ctx,
                    const Comunicacion& comunicacion, bool colectiva, bool todos) const;
    Entero minimoMPI(const Entero* A, const Contexto& ctx,
                    const Comunicacion& comunicacion, bool colectiva, bool todos) const;

private:
    Entero sumatoriaLocal(const Entero* A, const Contexto& ctx) const;
    Entero maximoLocal(const Entero* A, const Contexto& ctx) const;
    Entero minimoLocal(const Entero* A, const Contexto& ctx) const;
    // Cada nodo reduce con OpenMP; MPI combina los escalares fuera de la region.
    Entero reducirGlobal(Entero parcial, int opcion, bool todos) const;
    Entero consolidarReduccion(Entero parcial, int opcion, const Contexto& ctx,
                              const Comunicacion& comunicacion, bool colectiva, bool todos) const;
};
