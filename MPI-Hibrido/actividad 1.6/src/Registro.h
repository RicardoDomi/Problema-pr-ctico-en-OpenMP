#ifndef REGISTRO_H
#define REGISTRO_H

#include <fstream>
#include <string>

class Registro {
public:
    Registro(const std::string& directorio, const std::string& equipo, int rank);
    void escribir(const std::string& mensaje, bool mostrarEnConsola = true);

private:
    std::ofstream archivo_;
    std::string prefijo_;
};

#endif
