#ifndef ARREGLO_DINAMICO_H
#define ARREGLO_DINAMICO_H

#include <cstddef>

class ArregloDinamico {
public:
    ArregloDinamico();
    explicit ArregloDinamico(std::size_t cantidad);
    ~ArregloDinamico();

    ArregloDinamico(const ArregloDinamico&) = delete;
    ArregloDinamico& operator=(const ArregloDinamico&) = delete;

    void reservar(std::size_t cantidad);
    void copiarDesde(const ArregloDinamico& otro);
    int* datos();
    const int* datos() const;
    std::size_t cantidad() const;

private:
    int* datos_;
    std::size_t cantidad_;
};

#endif
