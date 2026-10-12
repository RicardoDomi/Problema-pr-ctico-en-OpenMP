#ifndef REGISTRO_H
#define REGISTRO_H

#include <fstream>
#include <string>

class Registro {
public:
    Registro(const std::string& directorio, const std::string& equipo, int rank);
    ~Registro();

    void general(const std::string& mensaje, bool consola = true);
    void busqueda(int hilo, long long inicio, long long fin,
                  const std::string& algoritmo, long long resultado,
                  bool consola);
    const std::string& ruta() const;

private:
    std::string prefijo(int hilo, long long inicio, long long fin) const;
    void escribir(const std::string& linea, bool consola);

    std::string equipo_;
    int rank_;
    std::string ruta_;
    std::ofstream archivo_;
};

#endif
