#include "file_benchmark.hpp"

#include <iomanip>
#include <iostream>

int main(int argc, char const* argv[])
{
    constexpr size_t kFileSize = 2UL << 30;
    FileBenchmark    fw("test.bin", kFileSize, 0xAA, false);

    constexpr size_t kBufferSize = sizeof(size_t);
    uint8_t          buffer[kBufferSize];
    off_t            loc;

    fw.random_aligned_read(buffer, kBufferSize, &loc);

    size_t content = *reinterpret_cast<size_t*>(buffer);

    std::cout << "Location\t " << loc / sizeof(size_t) << "\n";
    std::cout << "Content\t " << content << "\n";
    // std::ios_base::fmtflags f(std::cout.flags());
    // std::cout << std::showbase << std::setw(4) << std::hex;

    // for (size_t i = 0; i < kBufferSize; i++) {
    //     std::cout << int(buffer[i]) << " ";
    // }
    // std::cout << "\n";

    // std::cout.flags(f);

    return 0;
}
