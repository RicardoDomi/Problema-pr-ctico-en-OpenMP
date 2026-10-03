#include "CuentaBancaria.h"

#include <cstdio>

CuentaBancaria::CuentaBancaria(double saldoInicial) : saldo_(saldoInicial) {}

RespuestaServidor CuentaBancaria::procesar(const Transaccion& transaccion) {
    RespuestaServidor respuesta{};
    respuesta.aprobada = true;

    switch (transaccion.tipo_operacion) {
        case DEPOSITO:
            saldo_ += transaccion.monto;
            std::snprintf(respuesta.mensaje, sizeof(respuesta.mensaje),
                          "Deposito aprobado");
            break;
        case RETIRO:
            if (saldo_ >= transaccion.monto) {
                saldo_ -= transaccion.monto;
                std::snprintf(respuesta.mensaje, sizeof(respuesta.mensaje),
                              "Retiro aprobado");
            } else {
                respuesta.aprobada = false;
                std::snprintf(respuesta.mensaje, sizeof(respuesta.mensaje),
                              "Retiro rechazado por fondos insuficientes");
            }
            break;
        case CONSULTA:
            std::snprintf(respuesta.mensaje, sizeof(respuesta.mensaje),
                          "Consulta realizada");
            break;
        default:
            respuesta.aprobada = false;
            std::snprintf(respuesta.mensaje, sizeof(respuesta.mensaje),
                          "Tipo de operacion invalido");
            break;
    }

    respuesta.saldo_resultante = saldo_;
    return respuesta;
}

double CuentaBancaria::saldo() const { return saldo_; }
