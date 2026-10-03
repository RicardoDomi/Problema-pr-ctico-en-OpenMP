#pragma once

#include "Mensajes.h"

class Registro;

class ClienteBancario {
    int rank_;
    Registro& registro_;
    const TiposMPI& tiposMPI_;
    Transaccion* historialTransacciones_ = nullptr;
    RespuestaServidor* historialRespuestas_ = nullptr;
    int cantidad_ = 0;

    void liberarHistorial();
    void generarTransacciones(int cantidad);

public:
    ClienteBancario(int rank, Registro& registro, const TiposMPI& tiposMPI);
    ~ClienteBancario();

    ClienteBancario(const ClienteBancario&) = delete;
    ClienteBancario& operator=(const ClienteBancario&) = delete;

    void ejecutar(int cantidad, bool salidaDetallada);
};
