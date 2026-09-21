CXX = mpic++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic

PROCESSES ?= 5
THREADS ?= 2

SEND_SOURCE = memoria_mpi.cpp
SEND_TARGET = memoria_mpi
SCATTER_SOURCE = memoria_scatter_gather.cpp
SCATTER_TARGET = memoria_scatter_gather

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
LIBOMP_PREFIX := $(shell brew --prefix libomp)
OPENMP_FLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include \
	-L$(LIBOMP_PREFIX)/lib -Wl,-rpath,$(LIBOMP_PREFIX)/lib
OPENMP_LIBS = -lomp
else
OPENMP_FLAGS = -fopenmp
OPENMP_LIBS =
endif

.PHONY: all run-send run-scatter clean

all: $(SEND_TARGET) $(SCATTER_TARGET)

$(SEND_TARGET): $(SEND_SOURCE)
	$(CXX) $(CXXFLAGS) $(OPENMP_FLAGS) $< $(OPENMP_LIBS) -o $@

$(SCATTER_TARGET): $(SCATTER_SOURCE)
	$(CXX) $(CXXFLAGS) $(OPENMP_FLAGS) $< $(OPENMP_LIBS) -o $@

run-send: $(SEND_TARGET)
	OMP_NUM_THREADS=$(THREADS) mpirun -np $(PROCESSES) ./$(SEND_TARGET)

run-scatter: $(SCATTER_TARGET)
	OMP_NUM_THREADS=$(THREADS) mpirun -np $(PROCESSES) ./$(SCATTER_TARGET)

clean:
	rm -rf $(SEND_TARGET) $(SCATTER_TARGET) \
		$(SEND_TARGET).dSYM $(SCATTER_TARGET).dSYM
