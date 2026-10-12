#include "Comunicacion.h"
#include <algorithm>

const char* nombreVersion(Version version) {
    if (version == Version::SendRecv) return "Send/Recv";
    if (version == Version::ScatterGather) return "Scatter/Gather";
    return "Reduce/Allreduce";
}

void Comunicacion::distribuir(const Entero* global, Entero* local, int etiqueta) const {
    if (version == Version::SendRecv) {
        if (rank == 0) {
            for (int destino = 1; destino < procesos; ++destino)
                MPI_Send(global + static_cast<std::size_t>(destino) * bloque, bloque,
                         MPI_LONG_LONG_INT, destino, etiqueta, MPI_COMM_WORLD);
        } else {
            MPI_Recv(local, bloque, MPI_LONG_LONG_INT, 0, etiqueta, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Scatter(global, bloque, MPI_LONG_LONG_INT, local, bloque,
                    MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);
    }
}

void Comunicacion::reunir(const Entero* local, Entero* global, int etiqueta) const {
    if (version == Version::SendRecv) {
        if (rank == 0) {
            for (int origen = 1; origen < procesos; ++origen)
                MPI_Recv(global + static_cast<std::size_t>(origen) * bloque, bloque,
                         MPI_LONG_LONG_INT, origen, etiqueta, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Send(local, bloque, MPI_LONG_LONG_INT, 0, etiqueta, MPI_COMM_WORLD);
        }
    } else {
        MPI_Gather(local, bloque, MPI_LONG_LONG_INT, global, bloque,
                   MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);
    }
}

Entero Comunicacion::reunirParcial(Entero parcial, int opcion) const {
    Entero* parciales = rank == 0 ? new Entero[procesos]{} : nullptr;
    if (version == Version::SendRecv) {
        if (rank == 0) {
            parciales[0] = parcial;
            for (int origen = 1; origen < procesos; ++origen)
                MPI_Recv(parciales + origen, 1, MPI_LONG_LONG_INT, origen, 30,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Send(&parcial, 1, MPI_LONG_LONG_INT, 0, 30, MPI_COMM_WORLD);
        }
    } else {
        MPI_Gather(&parcial, 1, MPI_LONG_LONG_INT, parciales, 1,
                   MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);
    }
    Entero global = parcial;
    if (rank == 0) {
        for (int p = 1; p < procesos; ++p) {
            if (opcion == 10) global = std::max(global, parciales[p]);
            else if (opcion == 11) global = std::min(global, parciales[p]);
            else global += parciales[p];
        }
    }
    delete[] parciales;
    return global;
}
