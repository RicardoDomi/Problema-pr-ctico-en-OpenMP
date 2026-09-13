#ifndef BUSQUEDA_EXHAUSTIVA_H
#define BUSQUEDA_EXHAUSTIVA_H

#include <string>

class BusquedaExhaustiva
{
private:
    char* caracteres;
    int cantidadCaracteres;
    int longitud;

    bool caracterValido(char caracter);

public:
   
    BusquedaExhaustiva(int longitudClave);


    ~BusquedaExhaustiva();

    unsigned long long calcularCombinaciones();

  
    bool validarClave(std::string clave);

  
    std::string numeroAClave(unsigned long long numero);

    double busquedaSecuencial(
        std::string objetivo,
        unsigned long long& revisadas
    );

    double busquedaParalela(
        std::string objetivo,
        int& hiloGanador,
        unsigned long long& revisadasTotales,
        int& hilosUtilizados
    );
};

#endif