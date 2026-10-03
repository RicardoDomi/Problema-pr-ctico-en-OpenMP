#pragma once

#include <mpi.h>

enum TipoOperacion {
    FINALIZAR = 0,
    DEPOSITO = 1,
    RETIRO = 2,
    CONSULTA = 3
};

constexpr int TAG_TRANSACCION = 100;
constexpr int TAG_RESPUESTA = 101;
constexpr int TAG_FIN = 102;

struct Transaccion {
    int id_cliente;
    int id_hilo_openmp;
    int tipo_operacion;
    double monto;
};

struct RespuestaServidor {
    bool aprobada;
    double saldo_resultante;
    char mensaje[100];
};

class TiposMPI {
    MPI_Datatype tipoTransaccion_ = MPI_DATATYPE_NULL;
    MPI_Datatype tipoRespuesta_ = MPI_DATATYPE_NULL;

public:
    TiposMPI();
    ~TiposMPI();

    TiposMPI(const TiposMPI&) = delete;
    TiposMPI& operator=(const TiposMPI&) = delete;

    MPI_Datatype transaccion() const;
    MPI_Datatype respuesta() const;
};

const char* nombreOperacion(int tipo);
