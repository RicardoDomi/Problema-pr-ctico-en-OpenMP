#include <mpi.h>
#include <omp.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
//Castolo Gonzales Jorge Natanael.
//Domínguez Alcalá Ricardo.
//Silva Montes Diego Eduardo
using Number = unsigned long long;
enum { RANGE = 10, FOUND, STOP, COMPLETE };

void text(Number index, char* buffer, int length) {
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = length - 1; i >= 0; --i) {
        buffer[i] = alphabet[index % 36];
        index /= 36;
    }
    buffer[length] = '\0';
}

void report(std::ofstream& log, const std::string& message) {
#pragma omp critical(output)
    {
        std::cout << message << std::endl;
        log << message << std::endl;
    }
}

void worker(const std::string& key, int rank, const std::string& host,
            std::ofstream& log) {
    auto range = std::make_unique<Number[]>(2);
    MPI_Recv(range.get(), 2, MPI_UNSIGNED_LONG_LONG, 0, RANGE,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    report(log, "[RANGO] " + (range[0] < range[1]
        ? "[" + std::to_string(range[0]) + ", " + std::to_string(range[1] - 1) + "]"
        : std::string("Vacio")));
    int signal = 0;
    MPI_Request request;
    MPI_Irecv(&signal, 1, MPI_INT, 0, STOP, MPI_COMM_WORLD, &request);
    std::atomic<bool> stopped{false};
    const double start = omp_get_wtime();
    // Half-open internal ranges; bounded batches keep cancellation independent of N.
#pragma omp parallel shared(stopped, request)
    {
        const int tid = omp_get_thread_num();
        auto candidate = std::make_unique<char[]>(key.size() + 1);
        const std::string identity = "Host " + host + " rank " + std::to_string(rank)
            + " thread " + std::to_string(tid);
#pragma omp single
        report(log, "[Encabezado] Equipo: " + host + " | Rango MPI: " + std::to_string(rank)
            + " | Hilos OpenMP activos: " + std::to_string(omp_get_num_threads()));
        report(log, "[THREAD INIT] " + identity + " team " + std::to_string(omp_get_num_threads()));
        for (Number base = range[0]; base < range[1];) {
            const Number end = base + std::min<Number>(4096, range[1] - base);
#pragma omp for schedule(dynamic, 32)
            for (Number index = base; index < end; ++index) {
                if (stopped.load()) continue;
                if ((index - base) % 32 == 0) {
#pragma omp critical(mpi)
                    {
                        int received = 0;
                        MPI_Test(&request, &received, MPI_STATUS_IGNORE);
                        if (received) stopped.store(true);
                    }
                }
                if (stopped.load()) continue;
                text(index, candidate.get(), static_cast<int>(key.size()));
                if (key.size() == 3)
                    report(log, "[CANDIDATE] [Equipo: " + host + "] [Nodo MPI: " + std::to_string(rank)
                        + "] [Hilo OpenMP: " + std::to_string(tid) + "] [Candidato: "
                        + candidate.get() + "] [Estado: Evaluando]");
                if (key == candidate.get() && !stopped.exchange(true)) {
                    report(log, "[FOUND] [EXITO] [Equipo: " + host + "] [Nodo MPI: " + std::to_string(rank)
                        + "] [Hilo OpenMP: " + std::to_string(tid) + "] [Clave Encontrada: \""
                        + candidate.get() + "\"] [Tiempo Parcial: " + std::to_string(omp_get_wtime() - start) + "s]");
#pragma omp critical(mpi)
                    MPI_Ssend(&tid, 1, MPI_INT, 0, FOUND, MPI_COMM_WORLD);
                }
            }
            // All threads make the same outer-loop decision after the work barrier.
#pragma omp single
            {
                int received = 0;
#pragma omp critical(mpi)
                MPI_Test(&request, &received, MPI_STATUS_IGNORE);
                if (received) stopped.store(true);
                range[0] = stopped.load() ? range[1] : end;
            }
            base = range[0];
        }
        report(log, "[THREAD END] " + identity);
    }
    MPI_Send(&rank, 1, MPI_INT, 0, COMPLETE, MPI_COMM_WORLD);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    report(log, "[SEARCH FINISH] rank " + std::to_string(rank));
}

int main(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // Worker-thread MPI calls never overlap; the main thread is idle in the region.
    if (provided < MPI_THREAD_SERIALIZED) {
        if (rank == 0) std::cerr << "MPI_THREAD_SERIALIZED or MULTIPLE required.\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    auto hostname = std::make_unique<char[]>(MPI_MAX_PROCESSOR_NAME + 1);
    int hostlength;
    MPI_Get_processor_name(hostname.get(), &hostlength);
    hostname[hostlength] = '\0';
    const std::string host(hostname.get());
    const int stride = MPI_MAX_PROCESSOR_NAME + 1;
    auto hosts = std::make_unique<char[]>(static_cast<std::size_t>(size) * stride);
    MPI_Gather(hostname.get(), stride, MPI_CHAR, hosts.get(), stride, MPI_CHAR, 0, MPI_COMM_WORLD);
    std::ofstream log("log_equipo_" + host + "_nodo_" + std::to_string(rank) + ".txt");
    if (!log) MPI_Abort(MPI_COMM_WORLD, 2);
    report(log, "[INIT] Host " + host + " rank " + std::to_string(rank)
        + " MPI thread level " + std::to_string(provided));
    report(log, "[Encabezado] Equipo: " + host + " | Rango MPI: " + std::to_string(rank)
        + " | Hilos OpenMP activos al inicio: 1");
    std::string key;
    Number total = 0;
    double sequential = -1, parallel = -1;
    for (;;) {
        int option = 5;
        if (rank == 0) {
            report(log, "\nCastolo Gonzales Jorge Natanael.\n"
                "Domínguez Alcalá Ricardo.\n"
                "Silva Montes Diego Eduardo\n"
                "\n--- Menu ---\n1. Ingresar clave personalizada en mayusculas (A-Z, 0-9).\n"
                "2. Ejecutar busqueda Secuencial (1 nodo, 1 hilo - Linea Base).\n"
                "3. Ejecutar busqueda Paralelo Distribuido (MPI + OpenMP).\n"
                "4. Mostrar tabla comparativa de tiempos y Speedup.\n5. Salir.\nSeleccione una opcion:");
            std::string line;
            if (std::getline(std::cin, line)) {
                option = line.size() == 1 && line[0] >= '1' && line[0] <= '5' ? line[0] - '0' : 0;
                if (!option) report(log, "Opcion invalida.");
            }
        }
        MPI_Bcast(&option, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (option == 5) break;
        if (option == 1) {
            int length = 0;
            if (rank == 0) {
                for (;;) {
                    report(log, "Ingrese clave A-Z, 0-9 (1 a 12 caracteres):");
                    std::string input;
                    if (!std::getline(std::cin, input)) { length = -1; break; }
                    Number count = 1;
                    bool valid = !input.empty() && input.size() <= 12
                        && input.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") == std::string::npos;
                    for (std::size_t i = 0; valid && i < input.size(); ++i) {
                        if (count > std::numeric_limits<Number>::max() / 36) valid = false;
                        else count *= 36;
                    }
                    if (!valid) { report(log, "Clave invalida: limite numerico 36^12; use A-Z, 0-9."); continue; }
                    key = input; total = count; length = static_cast<int>(key.size());
                    sequential = parallel = -1;
                    break;
                }
            }
            MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (length < 0) break;
            key.resize(length);
            MPI_Bcast(key.data(), length, MPI_CHAR, 0, MPI_COMM_WORLD);
            MPI_Bcast(&total, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
        }
        if ((option == 2 || option == 3) && key.empty()) {
            if (rank == 0) report(log, "Primero ingrese una clave.");
            continue;
        }
        if (option == 2 && rank == 0) {
            report(log, "[SEQUENTIAL INIT] rank 0 thread 0 team 1");
            auto candidate = std::make_unique<char[]>(key.size() + 1);
            const double start = MPI_Wtime();
            for (Number i = 0; i < total; ++i) {
                text(i, candidate.get(), static_cast<int>(key.size()));
                if (key.size() == 3) report(log, "[CANDIDATE] rank 0 thread 0 index " + std::to_string(i) + " key " + candidate.get());
                if (key == candidate.get()) break;
            }
            sequential = MPI_Wtime() - start;
            report(log, "[SEQUENTIAL FOUND] key " + key + " seconds " + std::to_string(sequential));
            report(log, "[SEQUENTIAL END] rank 0 thread 0");
        }
        if (option == 3) {
            if (size < 2) {
                if (rank == 0) report(log, "Paralelo requiere al menos 2 procesos MPI.");
                continue;
            }
            if (rank != 0) worker(key, rank, host, log);
            else {
                const double start = MPI_Wtime();
                auto range = std::make_unique<Number[]>(2);
                Number cursor = 0;
                for (int p = 1; p < size; ++p) {
                    const Number count = total / (size - 1) + (static_cast<Number>(p - 1) < total % (size - 1));
                    range[0] = cursor; range[1] = cursor + count; cursor += count;
                    report(log, "[RANGE] rank " + std::to_string(p) + (count ? " inclusive [" + std::to_string(range[0]) + ", " + std::to_string(range[1] - 1) + "]" : " empty"));
                    MPI_Send(range.get(), 2, MPI_UNSIGNED_LONG_LONG, p, RANGE, MPI_COMM_WORLD);
                }
                auto requests = std::make_unique<MPI_Request[]>(size - 1);
                int completed = 0, signal = 1;
                bool stopping = false;
                while (completed < size - 1) {
                    int ready;
                    MPI_Status status;
                    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &ready, &status);
                    if (!ready) continue;
                    int value;
                    MPI_Recv(&value, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (status.MPI_TAG == COMPLETE) ++completed;
                    if (status.MPI_TAG == FOUND && !stopping) {
                        parallel = MPI_Wtime() - start;
                        report(log, "[WINNER] Host " + std::string(hosts.get() + status.MPI_SOURCE * stride)
                            + " rank " + std::to_string(status.MPI_SOURCE) + " thread " + std::to_string(value)
                            + " key " + key + " seconds " + std::to_string(parallel));
                    }
                    if (!stopping && (status.MPI_TAG == FOUND || completed == size - 1)) {
                        stopping = true;
                        for (int p = 1; p < size; ++p)
                            MPI_Isend(&signal, 1, MPI_INT, p, STOP, MPI_COMM_WORLD, &requests[p - 1]);
                    }
                }
                MPI_Waitall(size - 1, requests.get(), MPI_STATUSES_IGNORE);
                report(log, "[SEARCH FINISH] master drained all workers");
            }
        }
        if (option == 4 && rank == 0) {
            report(log, "Modo | Tiempo (s)\nSecuencial | " + (sequential < 0 ? std::string("no ejecutado") : std::to_string(sequential))
                + "\nMPI + OpenMP | " + (parallel < 0 ? std::string("no ejecutado") : std::to_string(parallel)));
            report(log, sequential > 0 && parallel > 0 ? "Speedup: " + std::to_string(sequential / parallel) : "Speedup no disponible.");
        }
    }
    report(log, "[FIN] rank " + std::to_string(rank));
    MPI_Finalize();
}
