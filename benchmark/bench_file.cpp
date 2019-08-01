#include "file_benchmark.hpp"

#include <benchmark/benchmark.h>

#include <vector>

static void UnalignedRead(benchmark::State& state)
{
    const size_t bench_size = state.range(0);
    const size_t read_size  = state.range(1);


    std::string filename = "bench_read";


    FileBenchmark fb(filename, bench_size, 0xAA, false);

    std::vector<uint8_t> buffer(read_size);

    for (auto _ : state) {
        fb.random_unaligned_read(buffer.data(), read_size);
    }
    state.SetBytesProcessed(read_size * state.iterations());
}

BENCHMARK(UnalignedRead)
    // ->Args({1UL << 10, 8})
    // ->Args({1UL << 20, 8})
    // ->Args({1UL << 30, 8})
    // ->Args({2UL << 30, 8})
    // ->Args({4UL << 30, 8})
    // ->Args({8UL << 30, 8})
    // ->Args({32UL << 30, 8})
    // ->Args({32UL << 30, 64})
    // ->Args({32UL << 30, 4UL << 10}) // 4KB reads
    // ->Args({32UL << 30, 8UL << 10})
    // ->Args({32UL << 30, 1UL << 16})
    // ->Args({32UL << 30, 1UL << 17})
    // ->Args({32UL << 30, 1UL << 18})
    // ->Args({32UL << 30, 1UL << 19})
    ->Args({32UL << 30, 1UL << 20}) // 1MB reads
    ->Args({32UL << 30, 2UL << 20})
    ->Args({32UL << 30, 4UL << 20})
    ->Args({32UL << 30, 16UL << 20})
    ->Args({32UL << 30, 32UL << 20});
//->Iterations(500);


static void DirectUnalignedRead(benchmark::State& state)
{
    const size_t bench_size = state.range(0);
    const size_t read_size  = state.range(1);


    std::string filename = "bench_read";


    FileBenchmark fb(filename, bench_size, 0xAA, true);

    std::vector<uint8_t> buffer(read_size);

    for (auto _ : state) {
        fb.random_unaligned_read(buffer.data(), read_size);
    }
    state.SetBytesProcessed(read_size * state.iterations());
}

BENCHMARK(DirectUnalignedRead)
    // ->Args({1UL << 10, 8})
    // ->Args({1UL << 20, 8})
    // ->Args({1UL << 30, 8})
    // ->Args({2UL << 30, 8})
    // ->Args({4UL << 30, 8})
    // ->Args({8UL << 30, 8})
    // ->Args({32UL << 30, 8})
    // ->Args({32UL << 30, 64})
    // ->Args({32UL << 30, 4UL << 10}) // 4KB reads
    // ->Args({32UL << 30, 8UL << 10})
    // ->Args({32UL << 30, 1UL << 16})
    // ->Args({32UL << 30, 1UL << 17})
    // ->Args({32UL << 30, 1UL << 18})
    // ->Args({32UL << 30, 1UL << 19})
    ->Args({32UL << 30, 1UL << 20}) // 1MB reads
    ->Args({32UL << 30, 2UL << 20})
    ->Args({32UL << 30, 4UL << 20})
    ->Args({32UL << 30, 16UL << 20})
    ->Args({32UL << 30, 32UL << 20});

BENCHMARK_MAIN();
