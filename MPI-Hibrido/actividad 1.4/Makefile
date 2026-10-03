CXX = mpic++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
TARGET = cuenta_bancaria
SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.h)
PROCESSES ?= 5
THREADS ?= 2
MPIRUN ?= mpirun
LOG_DIR ?= logs/local

ifeq ($(shell uname -s),Darwin)
LIBOMP_PREFIX ?= $(firstword $(wildcard /opt/homebrew/opt/libomp /usr/local/opt/libomp))
OPENMP_FLAGS = -Xpreprocessor -fopenmp -I$(LIBOMP_PREFIX)/include
OPENMP_LIBS = -L$(LIBOMP_PREFIX)/lib -Wl,-rpath,$(LIBOMP_PREFIX)/lib -lomp
else
OPENMP_FLAGS = -fopenmp
OPENMP_LIBS =
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(OPENMP_FLAGS) $(SOURCES) $(OPENMP_LIBS) -o $@

run: $(TARGET)
	OMP_NUM_THREADS=$(THREADS) $(MPIRUN) -np $(PROCESSES) ./$(TARGET) --logs $(LOG_DIR)

clean:
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM
