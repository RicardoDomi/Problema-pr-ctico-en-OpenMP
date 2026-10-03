CXX = mpic++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
PROCESSES ?= 5
THREADS ?= 2
N ?= 40
MPIRUN ?= mpirun
SOURCES = src/main.cpp src/OperacionesArreglos.cpp src/Comunicacion.cpp src/Registro.cpp
HEADERS = $(wildcard src/*.h)
TARGETS = arreglos_send_recv arreglos_scatter_gather arreglos_reduce

ifeq ($(shell uname -s),Darwin)
LIBOMP_PREFIX ?= $(firstword $(wildcard /opt/homebrew/opt/libomp /usr/local/opt/libomp))
OPENMP_FLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include
OPENMP_LIBS = -L$(LIBOMP_PREFIX)/lib -Wl,-rpath,$(LIBOMP_PREFIX)/lib -lomp
else
OPENMP_FLAGS = -fopenmp
OPENMP_LIBS =
endif

.PHONY: all run-send run-scatter run-reduce clean
all: $(TARGETS)

arreglos_send_recv: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(OPENMP_FLAGS) -DVERSION_PREDETERMINADA=1 $(SOURCES) $(OPENMP_LIBS) -o $@
arreglos_scatter_gather: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(OPENMP_FLAGS) -DVERSION_PREDETERMINADA=2 $(SOURCES) $(OPENMP_LIBS) -o $@
arreglos_reduce: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(OPENMP_FLAGS) -DVERSION_PREDETERMINADA=3 $(SOURCES) $(OPENMP_LIBS) -o $@

run-send: arreglos_send_recv
	OMP_NUM_THREADS=$(THREADS) $(MPIRUN) -np $(PROCESSES) ./arreglos_send_recv --n $(N)
run-scatter: arreglos_scatter_gather
	OMP_NUM_THREADS=$(THREADS) $(MPIRUN) -np $(PROCESSES) ./arreglos_scatter_gather --n $(N)
run-reduce: arreglos_reduce
	OMP_NUM_THREADS=$(THREADS) $(MPIRUN) -np $(PROCESSES) ./arreglos_reduce --n $(N)
clean:
	rm -f $(TARGETS)
	rm -rf $(addsuffix .dSYM,$(TARGETS))
