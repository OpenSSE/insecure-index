#include "file_benchmark.hpp"

#include <iomanip>
#include <iostream>

int main(int argc, char const* argv[])
{
    constexpr size_t kFileSize = 2UL << 30;
    FileBenchmark    fw("test.bin", kFileSize, 0xAA, false);

    constexpr size_t kBufferSize = 10;
    uint8_t          buffer[kBufferSize];

    fw.random_unaligned_read(buffer, kBufferSize);

    std::ios_base::fmtflags f(std::cout.flags());
    std::cout << std::showbase << std::setw(4) << std::hex;

    for (size_t i = 0; i < kBufferSize; i++) {
        std::cout << int(buffer[i]) << " ";
    }
    std::cout << "\n";

    std::cout.flags(f);

    return 0;
}
