#include "zipfian_distribution.hpp"

#include <benchmark/benchmark.h>

namespace sse {


static void Zipf_sample(benchmark::State& state)
{
    std::random_device            rnd;
    std::mt19937_64               rnd_gen(rnd());
    ZipfianDistribution<uint64_t> zipf_dist(1.2, 0, state.range(0) - 1);


    for (auto _ : state) {
        benchmark::DoNotOptimize(zipf_dist(rnd_gen));
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(Zipf_sample)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000)
    ->Arg(10000000);


} // namespace sse


BENCHMARK_MAIN();
