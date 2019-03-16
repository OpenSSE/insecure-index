#include "zipfian_distribution.hpp"

#include <algorithm>
#include <array>
#include <random>

#include <gtest/gtest.h>

namespace sse {

TEST(Zipf, basic)
{
    static constexpr size_t test_count = 1e5;
    static constexpr size_t max_value  = 9;

    std::random_device                  rd;
    std::mt19937                        gen(rd());
    ZipfianDistribution<size_t, double> zipf(1.2, 0, max_value);
    std::array<size_t, max_value + 1>   counts;
    std::fill(counts.begin(), counts.end(), 0);

    for (size_t i = 0; i < test_count; i++) {
        auto bucket = zipf(gen);
        EXPECT_GE(bucket, zipf.min());
        EXPECT_LE(bucket, zipf.max());
        counts[bucket - zipf.min()]++;
    }

    for (size_t i = 0; i < counts.size(); i++) {
        std::cerr << "counts[" << i << "] = \t" << counts[i] << "\n";
    }
}
} // namespace sse
