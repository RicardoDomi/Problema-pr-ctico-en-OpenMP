#pragma once

#include "Mensajes.h"

class CuentaBancaria {
    double saldo_;

public:
    explicit CuentaBancaria(double saldoInicial);

    RespuestaServidor procesar(const Transaccion& transaccion);
    double saldo() const;
};
