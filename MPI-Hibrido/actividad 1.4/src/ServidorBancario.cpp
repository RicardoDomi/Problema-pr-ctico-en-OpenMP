#include "ServidorBancario.h"

#include "Registro.h"

#include <mpi.h>

#include <stdexcept>

ServidorBancario::ServidorBancario(CuentaBancaria& cuenta,
                                   Registro& registro,
                                   const TiposMPI& tiposMPI)
    : cuenta_(cuenta), registro_(registro), tiposMPI_(tiposMPI) {}

ServidorBancario::~ServidorBancario() {
    delete[] historialTransacciones_;
    delete[] historialRespuestas_;
}

void ServidorBancario::prepararHistorial(int capacidad) {
    delete[] historialTransacciones_;
    delete[] historialRespuestas_;
    capacidad_ = capacidad;
    procesadas_ = 0;
    historialTransacciones_ = new Transaccion[capacidad_];
    historialRespuestas_ = new RespuestaServidor[capacidad_]{};
}

void ServidorBancario::ejecutar(int operacionesPorCliente, int totalProcesos) {
    const int clientes = totalProcesos - 1;
    prepararHistorial(operacionesPorCliente * clientes);
    int clientesTerminados = 0;

    while (clientesTerminados < clientes) {
        MPI_Status estado{};
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado);

        if (estado.MPI_TAG == TAG_FIN) {
            MPI_Recv(nullptr, 0, MPI_BYTE, estado.MPI_SOURCE, TAG_FIN,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++clientesTerminados;
            registro_.escribir("TAG_FIN recibido del cliente MPI " +
                               std::to_string(estado.MPI_SOURCE), false);
            continue;
        }

        if (estado.MPI_TAG != TAG_TRANSACCION) {
            throw std::runtime_error("El servidor recibio un tag MPI inesperado");
        }

        if (procesadas_ >= capacidad_) {
            throw std::runtime_error("El servidor recibio mas transacciones de las esperadas");
        }

        Transaccion solicitud{};
        MPI_Recv(&solicitud, 1, tiposMPI_.transaccion(), estado.MPI_SOURCE,
                 TAG_TRANSACCION, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        const RespuestaServidor respuesta = cuenta_.procesar(solicitud);

        historialTransacciones_[procesadas_] = solicitud;
        historialRespuestas_[procesadas_] = respuesta;
        ++procesadas_;

        registro_.transaccion(solicitud, respuesta,
                              "Servidor proceso y respondio", false);
        MPI_Send(&historialRespuestas_[procesadas_ - 1], 1, tiposMPI_.respuesta(),
                 estado.MPI_SOURCE, TAG_RESPUESTA, MPI_COMM_WORLD);
    }

    if (procesadas_ != capacidad_) {
        throw std::runtime_error("La cantidad procesada no coincide con la esperada");
    }
}

int ServidorBancario::procesadas() const { return procesadas_; }
