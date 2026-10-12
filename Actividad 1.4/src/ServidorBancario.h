#pragma once

#include "CuentaBancaria.h"
#include "Mensajes.h"

class Registro;

class ServidorBancario {
    CuentaBancaria& cuenta_;
    Registro& registro_;
    const TiposMPI& tiposMPI_;
    Transaccion* historialTransacciones_ = nullptr;
    RespuestaServidor* historialRespuestas_ = nullptr;
    int capacidad_ = 0;
    int procesadas_ = 0;

    void prepararHistorial(int capacidad);

public:
    ServidorBancario(CuentaBancaria& cuenta,
                     Registro& registro,
                     const TiposMPI& tiposMPI);
    ~ServidorBancario();

    ServidorBancario(const ServidorBancario&) = delete;
    ServidorBancario& operator=(const ServidorBancario&) = delete;

    void ejecutar(int operacionesPorCliente, int totalProcesos);
    int procesadas() const;
};
