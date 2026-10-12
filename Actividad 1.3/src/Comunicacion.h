#pragma once
#include "OperacionesArreglos.h"

enum class Version { SendRecv, ScatterGather, Reduce };
const char* nombreVersion(Version version);

// Igual cantidad enviada por proceso para usar Scatter/Gather literalmente.
// El primer bloque es auxiliar para rank 0; el relleno nunca se procesa.
class Comunicacion {
    int rank, procesos, bloque;
    Version version;
public:
    Comunicacion(int r, int p, int b, Version v) : rank(r), procesos(p), bloque(b), version(v) {}
    void distribuir(const Entero* global, Entero* local, int etiqueta) const;
    void reunir(const Entero* local, Entero* global, int etiqueta) const;
    Entero reunirParcial(Entero parcial, int opcion) const;
};
