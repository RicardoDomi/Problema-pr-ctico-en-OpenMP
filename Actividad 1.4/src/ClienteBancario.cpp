#include "ClienteBancario.h"

#include "Registro.h"

#include <mpi.h>
#include <omp.h>

#include <cstdlib>
#include <ctime>
#include <random>

ClienteBancario::ClienteBancario(int rank,
                                 Registro& registro,
                                 const TiposMPI& tiposMPI)
    : rank_(rank), registro_(registro), tiposMPI_(tiposMPI) {}

ClienteBancario::~ClienteBancario() { liberarHistorial(); }

void ClienteBancario::liberarHistorial() {
    delete[] historialTransacciones_;
    delete[] historialRespuestas_;
    historialTransacciones_ = nullptr;
    historialRespuestas_ = nullptr;
    cantidad_ = 0;
}

void ClienteBancario::generarTransacciones(int cantidad) {
    liberarHistorial();
    cantidad_ = cantidad;
    historialTransacciones_ = new Transaccion[cantidad_];
    historialRespuestas_ = new RespuestaServidor[cantidad_]{};

    const int maximoHilos = omp_get_max_threads();
    unsigned int* semillas = new unsigned int[maximoHilos];
    const unsigned int semillaProceso =
        static_cast<unsigned int>(std::time(nullptr)) ^
        (static_cast<unsigned int>(rank_) * 2654435761u);
    std::srand(semillaProceso);
    for (int hilo = 0; hilo < maximoHilos; ++hilo) {
        semillas[hilo] = static_cast<unsigned int>(std::rand()) ^
                         (static_cast<unsigned int>(hilo + 1) * 2246822519u);
    }

#pragma omp parallel
    {
        const int hilo = omp_get_thread_num();
        std::mt19937 generador(semillas[hilo]);
        std::uniform_int_distribution<int> operacionAleatoria(DEPOSITO, CONSULTA);
        std::uniform_int_distribution<int> montoEnCentavos(100, 200000);

#pragma omp for schedule(static)
        for (int indice = 0; indice < cantidad_; ++indice) {
            Transaccion transaccion{};
            transaccion.id_cliente = rank_;
            transaccion.id_hilo_openmp = hilo;
            transaccion.tipo_operacion = operacionAleatoria(generador);
            transaccion.monto = transaccion.tipo_operacion == CONSULTA
                                    ? 0.0
                                    : montoEnCentavos(generador) / 100.0;
            historialTransacciones_[indice] = transaccion;
        }
    }

    delete[] semillas;
}

void ClienteBancario::ejecutar(int cantidad, bool salidaDetallada) {
    generarTransacciones(cantidad);

    for (int indice = 0; indice < cantidad_; ++indice) {
        MPI_Send(&historialTransacciones_[indice], 1, tiposMPI_.transaccion(),
                 0, TAG_TRANSACCION, MPI_COMM_WORLD);
        MPI_Recv(&historialRespuestas_[indice], 1, tiposMPI_.respuesta(),
                 0, TAG_RESPUESTA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        registro_.transaccion(historialTransacciones_[indice],
                              historialRespuestas_[indice],
                              "Cliente envio solicitud y recibio respuesta",
                              salidaDetallada);
    }

    MPI_Send(nullptr, 0, MPI_BYTE, 0, TAG_FIN, MPI_COMM_WORLD);
    registro_.escribir("Senal TAG_FIN enviada al servidor", false);
}
