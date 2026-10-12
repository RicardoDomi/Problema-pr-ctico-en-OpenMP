#pragma once

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>

// Una instancia por proceso. Todos los mensajes propios pasan por este registro.
class Registro {
    std::ofstream archivo;
    std::mutex cerrojo;
    std::atomic<bool> fallo{false};
    std::string equipo;
    std::string pendientes;
    int rank;
    void imprimir(const char* texto, std::size_t longitud);
public:
    Registro(const std::string& directorio, const std::string& nombre, int proceso);
    void escribir(const std::string& texto, bool consola = true);
    void evento(const std::string& operacion, const std::string& valor,
                int hilo = -1, int posicion = -1, bool consola = true);
    void comprobar() const;
    // Se llama por todos los procesos desde el hilo principal, fuera de OpenMP.
    // El maestro presenta las lineas de los trabajadores sin mezclar fragmentos.
    void mostrarPendientes();
    // Para errores fatales, cuando ya no es posible sincronizar todos los nodos.
    void vaciarConsolaLocal();
    void cerrar();
};
