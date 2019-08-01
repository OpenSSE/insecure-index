#pragma once

#include <random>
#include <string>

class FileBenchmark
{
public:
    FileBenchmark(const std::string& filename,
                  size_t             size,
                  uint8_t            fill_byte,
                  bool               direct_io);

    ~FileBenchmark();


    size_t random_unaligned_read(uint8_t* buffer, size_t n_byte);
    size_t random_aligned_read(uint8_t* buffer, size_t n_byte);

private:
    void fill(uint8_t byte, size_t length);

    int             m_file_descriptor{-1};
    std::mt19937_64 m_random_generator;

    const size_t m_size;
};
