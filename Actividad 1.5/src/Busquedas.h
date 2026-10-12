#ifndef BUSQUEDAS_H
#define BUSQUEDAS_H

#include "Registro.h"

#include <cstddef>
#include <string>

long long busquedaSecuencialOpenMP(const int* datos, std::size_t cantidad,
                                  int target, long long offset,
                                  Registro& registro, const std::string& nombre,
                                  bool detallado);

long long busquedaBinariaOpenMP(const int* datos, std::size_t cantidad,
                               int target, long long offset,
                               Registro& registro, const std::string& nombre,
                               bool detallado);

void ordenarMergeSecuencial(int* datos, std::size_t cantidad, int* auxiliar);
void ordenarMergeOpenMP(int* datos, std::size_t cantidad, int* auxiliar);
bool estaOrdenado(const int* datos, std::size_t cantidad);

#endif
