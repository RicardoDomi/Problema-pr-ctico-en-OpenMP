#include "Busquedas.h"

#include <algorithm>
#include <climits>
#include <omp.h>

namespace {
constexpr std::size_t UMBRAL_ORDENAMIENTO = 4096;

void mezclar(int* datos, int* auxiliar, std::size_t inicio,
             std::size_t mitad, std::size_t fin) {
    std::size_t izquierda = inicio, derecha = mitad, salida = inicio;
    while (izquierda < mitad && derecha < fin) {
        auxiliar[salida++] = datos[izquierda] <= datos[derecha]
            ? datos[izquierda++] : datos[derecha++];
    }
    while (izquierda < mitad) auxiliar[salida++] = datos[izquierda++];
    while (derecha < fin) auxiliar[salida++] = datos[derecha++];
    for (std::size_t i = inicio; i < fin; ++i) datos[i] = auxiliar[i];
}

void mergeSecuencial(int* datos, int* auxiliar, std::size_t inicio, std::size_t fin) {
    if (fin - inicio <= 1) return;
    if (fin - inicio <= UMBRAL_ORDENAMIENTO) {
        std::sort(datos + inicio, datos + fin);
        return;
    }
    const std::size_t mitad = inicio + (fin - inicio) / 2;
    mergeSecuencial(datos, auxiliar, inicio, mitad);
    mergeSecuencial(datos, auxiliar, mitad, fin);
    mezclar(datos, auxiliar, inicio, mitad, fin);
}

void mergeParalelo(int* datos, int* auxiliar, std::size_t inicio,
                   std::size_t fin, int profundidad) {
    if (fin - inicio <= UMBRAL_ORDENAMIENTO || profundidad <= 0) {
        mergeSecuencial(datos, auxiliar, inicio, fin);
        return;
    }
    const std::size_t mitad = inicio + (fin - inicio) / 2;
    #pragma omp task shared(datos, auxiliar) firstprivate(inicio, mitad, profundidad)
    mergeParalelo(datos, auxiliar, inicio, mitad, profundidad - 1);
    #pragma omp task shared(datos, auxiliar) firstprivate(mitad, fin, profundidad)
    mergeParalelo(datos, auxiliar, mitad, fin, profundidad - 1);
    #pragma omp taskwait
    mezclar(datos, auxiliar, inicio, mitad, fin);
}

long long binariaPrimera(const int* datos, long long inicio, long long fin, int target) {
    long long resultado = -1;
    while (inicio <= fin) {
        const long long mitad = inicio + (fin - inicio) / 2;
        if (datos[mitad] >= target) {
            if (datos[mitad] == target) resultado = mitad;
            fin = mitad - 1;
        } else {
            inicio = mitad + 1;
        }
    }
    return resultado;
}
}

long long busquedaSecuencialOpenMP(const int* datos, std::size_t cantidad,
                                  int target, long long offset,
                                  Registro& registro, const std::string& nombre,
                                  bool detallado) {
    long long mejor = LLONG_MAX;
    #pragma omp parallel reduction(min:mejor)
    {
        const int hilo = omp_get_thread_num();
        const int hilos = omp_get_num_threads();
        const std::size_t inicio = cantidad * static_cast<std::size_t>(hilo) /
                                   static_cast<std::size_t>(hilos);
        const std::size_t finExclusivo = cantidad * static_cast<std::size_t>(hilo + 1) /
                                         static_cast<std::size_t>(hilos);
        long long encontrado = -1;
        for (std::size_t i = inicio; i < finExclusivo; ++i) {
            if (datos[i] == target) {
                encontrado = offset + static_cast<long long>(i);
                mejor = std::min(mejor, encontrado);
                break;
            }
        }
        registro.busqueda(hilo, offset + static_cast<long long>(inicio),
                          offset + static_cast<long long>(finExclusivo) - 1,
                          nombre, encontrado, detallado);
    }
    return mejor == LLONG_MAX ? -1 : mejor;
}

long long busquedaBinariaOpenMP(const int* datos, std::size_t cantidad,
                               int target, long long offset,
                               Registro& registro, const std::string& nombre,
                               bool detallado) {
    long long mejor = LLONG_MAX;
    #pragma omp parallel reduction(min:mejor)
    {
        const int hilo = omp_get_thread_num();
        const int hilos = omp_get_num_threads();
        const long long inicio = static_cast<long long>(cantidad) * hilo / hilos;
        const long long finExclusivo = static_cast<long long>(cantidad) * (hilo + 1) / hilos;
        long long encontrado = -1;
        if (inicio < finExclusivo && target >= datos[inicio] && target <= datos[finExclusivo - 1]) {
            const long long local = binariaPrimera(datos, inicio, finExclusivo - 1, target);
            if (local >= 0) {
                encontrado = offset + local;
                mejor = std::min(mejor, encontrado);
            }
        }
        registro.busqueda(hilo, offset + inicio, offset + finExclusivo - 1,
                          nombre, encontrado, detallado);
    }
    return mejor == LLONG_MAX ? -1 : mejor;
}

void ordenarMergeSecuencial(int* datos, std::size_t cantidad, int* auxiliar) {
    mergeSecuencial(datos, auxiliar, 0, cantidad);
}

void ordenarMergeOpenMP(int* datos, std::size_t cantidad, int* auxiliar) {
    int profundidad = 0;
    for (int hilos = omp_get_max_threads(); hilos > 1; hilos /= 2) ++profundidad;
    #pragma omp parallel
    {
        #pragma omp single
        mergeParalelo(datos, auxiliar, 0, cantidad, profundidad + 1);
    }
}

bool estaOrdenado(const int* datos, std::size_t cantidad) {
    int correcto = 1;
    #pragma omp parallel for reduction(&:correcto)
    for (long long i = 1; i < static_cast<long long>(cantidad); ++i)
        correcto &= datos[i - 1] <= datos[i];
    return correcto != 0;
}
