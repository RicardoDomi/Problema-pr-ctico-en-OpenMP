#include "Mensajes.h"

TiposMPI::TiposMPI() {
    Transaccion transaccionMuestra{};
    int longitudesTransaccion[4] = {1, 1, 1, 1};
    MPI_Aint desplazamientosTransaccion[4];
    MPI_Datatype tiposTransaccion[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE};
    MPI_Aint base = 0;

    MPI_Get_address(&transaccionMuestra, &base);
    MPI_Get_address(&transaccionMuestra.id_cliente, &desplazamientosTransaccion[0]);
    MPI_Get_address(&transaccionMuestra.id_hilo_openmp, &desplazamientosTransaccion[1]);
    MPI_Get_address(&transaccionMuestra.tipo_operacion, &desplazamientosTransaccion[2]);
    MPI_Get_address(&transaccionMuestra.monto, &desplazamientosTransaccion[3]);
    for (MPI_Aint& desplazamiento : desplazamientosTransaccion) desplazamiento -= base;

    MPI_Type_create_struct(4, longitudesTransaccion, desplazamientosTransaccion,
                           tiposTransaccion, &tipoTransaccion_);
    MPI_Type_commit(&tipoTransaccion_);

    RespuestaServidor respuestaMuestra{};
    int longitudesRespuesta[3] = {1, 1, 100};
    MPI_Aint desplazamientosRespuesta[3];
    MPI_Datatype tiposRespuesta[3] = {MPI_C_BOOL, MPI_DOUBLE, MPI_CHAR};

    MPI_Get_address(&respuestaMuestra, &base);
    MPI_Get_address(&respuestaMuestra.aprobada, &desplazamientosRespuesta[0]);
    MPI_Get_address(&respuestaMuestra.saldo_resultante, &desplazamientosRespuesta[1]);
    MPI_Get_address(&respuestaMuestra.mensaje, &desplazamientosRespuesta[2]);
    for (MPI_Aint& desplazamiento : desplazamientosRespuesta) desplazamiento -= base;

    MPI_Type_create_struct(3, longitudesRespuesta, desplazamientosRespuesta,
                           tiposRespuesta, &tipoRespuesta_);
    MPI_Type_commit(&tipoRespuesta_);
}

TiposMPI::~TiposMPI() {
    if (tipoTransaccion_ != MPI_DATATYPE_NULL) MPI_Type_free(&tipoTransaccion_);
    if (tipoRespuesta_ != MPI_DATATYPE_NULL) MPI_Type_free(&tipoRespuesta_);
}

MPI_Datatype TiposMPI::transaccion() const { return tipoTransaccion_; }

MPI_Datatype TiposMPI::respuesta() const { return tipoRespuesta_; }

const char* nombreOperacion(int tipo) {
    switch (tipo) {
        case DEPOSITO: return "Deposito";
        case RETIRO: return "Retiro";
        case CONSULTA: return "Consulta";
        case FINALIZAR: return "Finalizar";
        default: return "Desconocida";
    }
}
