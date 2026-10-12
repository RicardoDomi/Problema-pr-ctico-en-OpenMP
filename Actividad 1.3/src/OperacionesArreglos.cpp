#include "OperacionesArreglos.h"
#include "Comunicacion.h"
#include <omp.h>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <random>

namespace {
void detalle(const Contexto& ctx, int i, const char* op, Entero valor) {
    if (ctx.detallado)
        ctx.log.evento(op, std::to_string(valor), omp_get_thread_num(), ctx.inicio + i);
}
}

void OperacionesArreglos::crearArregloMPI(Entero*& A, Entero*& B, Entero*& C,
                                         int bloque, const Contexto& ctx) const {
    delete[] A; delete[] B; delete[] C;
    A = nullptr; B = nullptr; C = nullptr;
    A = new Entero[bloque]{};
    B = new Entero[bloque]{};
    C = new Entero[bloque]{};
    if (ctx.detallado)
        ctx.log.evento("Crear arreglos", "elementos locales=" + std::to_string(ctx.cantidad)
                       + ", inicio global=" + std::to_string(ctx.inicio));
}

void OperacionesArreglos::llenarSecuencial(Entero* A, Entero* B, const Contexto& ctx) const {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        A[i] = static_cast<Entero>(ctx.inicio) + i + 1;
        B[i] = static_cast<Entero>(ctx.total) - ctx.inicio - i;
        detalle(ctx, i, "Llenar secuencial A", A[i]);
        detalle(ctx, i, "Llenar secuencial B", B[i]);
    }
}

void OperacionesArreglos::llenarAleatorio(Entero* A, Entero* B, const Contexto& ctx) const {
    // srand se inicializa en el hilo principal una vez por proceso. rand nunca
    // se llama dentro de OpenMP: cada hilo tiene un motor independiente.
    const unsigned int base = static_cast<unsigned int>(std::rand());
    #pragma omp parallel
    {
        const int hilo = omp_get_thread_num();
        std::seed_seq semilla{ctx.semilla, base, static_cast<unsigned int>(ctx.rank),
                              static_cast<unsigned int>(hilo)};
        std::mt19937 generador(semilla);
        std::uniform_int_distribution<int> distribucion(1, 1000000);
        #pragma omp for schedule(static)
        for (int i = 0; i < ctx.cantidad; ++i) {
            A[i] = distribucion(generador);
            B[i] = distribucion(generador);
            detalle(ctx, i, "Llenar aleatorio A", A[i]);
            detalle(ctx, i, "Llenar aleatorio B", B[i]);
        }
    }
}

void OperacionesArreglos::sumarArreglosOpenMPI(const Entero* A, const Entero* B,
                                              Entero* C, const Contexto& ctx) const {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        C[i] = A[i] + B[i];
        detalle(ctx, i, "Suma", C[i]);
    }
}

void OperacionesArreglos::restarArreglosOpenMPI(const Entero* A, const Entero* B,
                                               Entero* C, const Contexto& ctx) const {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        C[i] = A[i] - B[i];
        detalle(ctx, i, "Resta", C[i]);
    }
}

void OperacionesArreglos::multiplicarArreglosOpenMPI(const Entero* A, const Entero* B,
                                                    Entero* C, const Contexto& ctx) const {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        C[i] = A[i] * B[i];
        detalle(ctx, i, "Multiplicacion", C[i]);
    }
}

void OperacionesArreglos::cuadradoArregloOpenMPI(const Entero* A, Entero* C,
                                                const Contexto& ctx) const {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        C[i] = A[i] * A[i];
        detalle(ctx, i, "Cuadrado", C[i]);
    }
}

Entero OperacionesArreglos::sumatoriaLocal(const Entero* A, const Contexto& ctx) const {
    Entero suma_local = 0;
    #pragma omp parallel for reduction(+:suma_local) schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        suma_local += A[i];
        detalle(ctx, i, "Sumatoria Local (aporte)", A[i]);
    }
    return suma_local;
}

Entero OperacionesArreglos::maximoLocal(const Entero* A, const Contexto& ctx) const {
    Entero max_local = std::numeric_limits<Entero>::lowest();
    #pragma omp parallel for reduction(max:max_local) schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        max_local = std::max(max_local, A[i]);
        detalle(ctx, i, "Maximo Local (candidato)", A[i]);
    }
    return max_local;
}

Entero OperacionesArreglos::minimoLocal(const Entero* A, const Contexto& ctx) const {
    Entero min_local = std::numeric_limits<Entero>::max();
    #pragma omp parallel for reduction(min:min_local) schedule(static)
    for (int i = 0; i < ctx.cantidad; ++i) {
        min_local = std::min(min_local, A[i]);
        detalle(ctx, i, "Minimo Local (candidato)", A[i]);
    }
    return min_local;
}

Entero OperacionesArreglos::reducirGlobal(Entero parcial, int opcion, bool todos) const {
    Entero global = 0;
    MPI_Op op = opcion == 10 ? MPI_MAX : opcion == 11 ? MPI_MIN : MPI_SUM;
    if (todos)
        MPI_Allreduce(&parcial, &global, 1, MPI_LONG_LONG_INT, op, MPI_COMM_WORLD);
    else
        MPI_Reduce(&parcial, &global, 1, MPI_LONG_LONG_INT, op, 0, MPI_COMM_WORLD);
    return global; // En Reduce, solo rank 0 puede utilizarlo.
}

Entero OperacionesArreglos::consolidarReduccion(Entero parcial, int opcion,
        const Contexto& ctx, const Comunicacion& comunicacion, bool colectiva, bool todos) const {
    const char* nombre = opcion == 8 ? "Sumatoria" : opcion == 9 ? "Promedio (sumatoria)"
                       : opcion == 10 ? "Maximo" : "Minimo";
    if (ctx.detallado)
        ctx.log.evento("Parcial " + std::string(nombre), std::to_string(parcial));
    // MPI se invoca despues de terminar la region paralela de reduccion local.
    return colectiva ? reducirGlobal(parcial, opcion, todos)
                     : comunicacion.reunirParcial(parcial, opcion);
}

Entero OperacionesArreglos::sumatoriaMPI(const Entero* A, const Contexto& ctx,
        const Comunicacion& comunicacion, bool colectiva, bool todos) const {
    return consolidarReduccion(sumatoriaLocal(A, ctx), 8, ctx, comunicacion, colectiva, todos);
}

double OperacionesArreglos::promedioMPI(const Entero* A, const Contexto& ctx,
        const Comunicacion& comunicacion, bool colectiva, bool todos) const {
    const Entero suma = consolidarReduccion(sumatoriaLocal(A, ctx), 9, ctx,
                                          comunicacion, colectiva, todos);
    if (ctx.rank == 0 || (colectiva && todos))
        return static_cast<double>(suma) / ctx.total;
    return 0.0; // En las variantes con resultado solo en 0, no se utiliza aqui.
}

Entero OperacionesArreglos::maximoMPI(const Entero* A, const Contexto& ctx,
        const Comunicacion& comunicacion, bool colectiva, bool todos) const {
    return consolidarReduccion(maximoLocal(A, ctx), 10, ctx, comunicacion, colectiva, todos);
}

Entero OperacionesArreglos::minimoMPI(const Entero* A, const Contexto& ctx,
        const Comunicacion& comunicacion, bool colectiva, bool todos) const {
    return consolidarReduccion(minimoLocal(A, ctx), 11, ctx, comunicacion, colectiva, todos);
}
