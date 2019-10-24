#include "file_benchmark.hpp"
#include "logger.hpp"

#include <omp.h>

#include <iomanip>
#include <iostream>

void run_benchmark(const std::string filename,
                   const size_t      file_size,
                   const bool        direct_access,
                   const size_t      read_size,
                   const size_t      n_iters,
                   const size_t      n_threads)
{
    FileBenchmark fw(filename, file_size, 0xAA, direct_access);

    std::vector<uint8_t*> buffers(n_threads);

    std::cerr << "Init buffers\n";

    for (auto& ptr : buffers) {
        int ret
            = posix_memalign((reinterpret_cast<void**>(&ptr)), 4096, read_size);

        if (ret != 0) {
            throw std::runtime_error("Unable to do an aligned allocation");
        }
    }

    std::cerr << "Start benchmark\n";
    std::string message
        = direct_access ? "parallel direct read" : "parallel cached read";
    sse::SearchBenchmark bench(message);

    {
#pragma omp parallel for num_threads(n_threads) schedule(dynamic)
        for (size_t i = 0; i < n_iters; i++) {
            int thread_num = omp_get_thread_num();

            // auto str = "Thread " + std::to_string(thread_num) + "/" +
            // std::to_string(omp_get_num_threads()) + "\n"; std::cerr << str;

            fw.random_aligned_read(buffers[thread_num], read_size);
        }
    }
    bench.stop(n_iters);
    bench.set_locality(n_threads);

    for (auto& ptr : buffers) {
        free(ptr);
    }
}


int main(int argc, char const* argv[])
{
    sse::Benchmark::set_log_to_console();

    const std::string filename  = "bench_read";
    const size_t      file_size = 32UL << 30;


    const size_t n_threads = 128;
    const size_t n_iters   = 100000;
    const size_t read_size = 4UL << 10;

    run_benchmark(filename, file_size, true, read_size, n_iters, n_threads);

    return 0;
}
