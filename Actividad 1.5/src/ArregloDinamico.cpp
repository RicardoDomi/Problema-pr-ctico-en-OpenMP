#include "ArregloDinamico.h"

#include <algorithm>

ArregloDinamico::ArregloDinamico() : datos_(nullptr), cantidad_(0) {}

ArregloDinamico::ArregloDinamico(std::size_t cantidad)
    : datos_(nullptr), cantidad_(0) {
    reservar(cantidad);
}

ArregloDinamico::~ArregloDinamico() {
    delete[] datos_;
}

void ArregloDinamico::reservar(std::size_t cantidad) {
    int* nuevos = cantidad == 0 ? nullptr : new int[cantidad];
    delete[] datos_;
    datos_ = nuevos;
    cantidad_ = cantidad;
}

void ArregloDinamico::copiarDesde(const ArregloDinamico& otro) {
    reservar(otro.cantidad_);
    std::copy(otro.datos_, otro.datos_ + otro.cantidad_, datos_);
}

int* ArregloDinamico::datos() { return datos_; }
const int* ArregloDinamico::datos() const { return datos_; }
std::size_t ArregloDinamico::cantidad() const { return cantidad_; }
