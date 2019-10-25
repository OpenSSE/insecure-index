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


    size_t random_unaligned_read(uint8_t* buffer,
                                 size_t   n_byte,
                                 off_t*   location);
    size_t random_aligned_read(uint8_t* buffer, size_t n_byte, off_t* location);


    size_t random_unaligned_read(uint8_t* buffer, size_t n_byte)
    {
        return random_unaligned_read(buffer, n_byte, nullptr);
    }
    size_t random_aligned_read(uint8_t* buffer, size_t n_byte)
    {
        return random_aligned_read(buffer, n_byte, nullptr);
    }

private:
    void fill(uint8_t byte, size_t length);
    void fill_consecutive(size_t start, size_t length);

    int             m_file_descriptor{-1};
    std::mt19937_64 m_random_generator;

    const size_t m_size;
};
