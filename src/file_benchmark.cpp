#include "file_benchmark.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>


bool write_wrapper(int fd, const void* buf, size_t nbyte)
{
    constexpr size_t LIMIT_2GB = 1UL << 31;
    constexpr size_t LIMIT_1GB = 1UL << 30;

    const char* src  = reinterpret_cast<const char*>(buf);
    size_t      left = nbyte;

    while (left != 0) {
        size_t bytes_to_write = left;
        if (bytes_to_write >= LIMIT_2GB) {
            bytes_to_write = LIMIT_1GB;
        }
        ssize_t done = write(fd, src, bytes_to_write);
        if (done < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        left -= done;
        src += done;
    }
    return true;
}


FileBenchmark::FileBenchmark(const std::string& filename,
                             size_t             size,
                             uint8_t            fill_byte,
                             bool               direct_io)
    : m_random_generator(std::random_device()()), m_size(size)
{
    int flags = (O_CREAT | O_RDWR);

    if (direct_io) {
#if !defined(OS_MACOSX) && !defined(OS_OPENBSD) && !defined(OS_SOLARIS)
        flags |= O_DIRECT;
#endif
    }

    int fd = open(filename.c_str(), flags, 0644);

    if (fd == -1) {
        std::cerr << "Error when opening file " << filename << "; errno "
                  << errno << "\n";
        throw std::runtime_error("Error when opening file " + filename
                                 + "; errno " + std::to_string(errno) + "("
                                 + strerror(errno) + ")");
    }

    if (direct_io) {
#ifdef OS_MACOSX
        if (fcntl(fd, F_NOCACHE, 1) == -1) {
            close(fd);
            throw std::runtime_error(
                "Error calling fcntl F_NOCACHE on file " + filename + "; errno "
                + std::to_string(errno) + "(" + strerror(errno) + ")");
        }
#endif
    }

    m_file_descriptor = fd;


    // get the file size using stat
    struct stat st;
    fstat(m_file_descriptor, &st);
    size_t original_size = st.st_size;

    std::cout << "File size: " << original_size << " bytes\n" << std::flush;

    if (original_size < size) {
        std::cout << "Filling " << size - original_size << " bytes..."
                  << std::flush;
        // get to the end of the file
        lseek(m_file_descriptor, 0UL, SEEK_END);
        // fill the file with the remaining bytes
        // fill(fill_byte, size - original_size);
        fill_consecutive(original_size / sizeof(size_t), size - original_size);
        // re-position the head at the beginning
        lseek(m_file_descriptor, 0UL, SEEK_SET);

        std::cout << "done" << std::endl;
    }
}

FileBenchmark::~FileBenchmark()
{
    close(m_file_descriptor);
}

void FileBenchmark::fill(uint8_t byte, size_t length)
{
    constexpr size_t kBufferSize = 4 << 20; // 4MB
    uint8_t          buffer[kBufferSize];
    memset(buffer, byte, kBufferSize);

    // write the file by using chunks of decreasing size
    size_t remaining_length = length;

    for (size_t chunk_length = kBufferSize; chunk_length != 0;
         chunk_length        = chunk_length / 2) {
        for (; remaining_length >= chunk_length;
             remaining_length -= chunk_length) {
            bool success
                = write_wrapper(m_file_descriptor, buffer, chunk_length);
            if (!success) {
                std::cerr << ("Error when writing file ; errno "
                              + std::to_string(errno) + "(" + strerror(errno)
                              + ")\n");
                throw std::runtime_error("Error when writing file ; errno "
                                         + std::to_string(errno) + "("
                                         + strerror(errno) + ")");
            }
        }
    }
}

void FileBenchmark::fill_consecutive(size_t start, size_t length)
{
    constexpr size_t kBufferSizeBytes = 4 << 20;                         // 4MB
    constexpr size_t kBufferNumElts = kBufferSizeBytes / sizeof(size_t); // 4MB
    size_t           buffer[kBufferNumElts];

    // init the buffer
    for (size_t counter = start, i = 0; i < kBufferNumElts; i++, counter++) {
        buffer[i] = counter;
    }

    size_t remaining_length = length;

    for (; remaining_length >= kBufferSizeBytes;
         remaining_length -= kBufferSizeBytes) {
        bool success
            = write_wrapper(m_file_descriptor, buffer, kBufferSizeBytes);
        if (!success) {
            std::cerr << ("Error when writing file ; errno "
                          + std::to_string(errno) + "(" + strerror(errno)
                          + ")\n");
            throw std::runtime_error("Error when writing file ; errno "
                                     + std::to_string(errno) + "("
                                     + strerror(errno) + ")");
        }

        // update the buffer
        for (size_t i = 0; i < kBufferNumElts; i++) {
            buffer[i] += kBufferNumElts;
        }
    }

    if (remaining_length > 0) {
        bool success
            = write_wrapper(m_file_descriptor, buffer, remaining_length);
        if (!success) {
            std::cerr << ("Error when writing file ; errno "
                          + std::to_string(errno) + "(" + strerror(errno)
                          + ")\n");
            throw std::runtime_error("Error when writing file ; errno "
                                     + std::to_string(errno) + "("
                                     + strerror(errno) + ")");
        }
    }
}

size_t FileBenchmark::random_unaligned_read(uint8_t* buffer,
                                            size_t   n_byte,
                                            off_t*   location)
{
    // chose a random position
    std::uniform_int_distribution<off_t> uniform_dist(0, m_size - n_byte);
    off_t offset = uniform_dist(m_random_generator);

    if (location != nullptr) {
        *location = offset;
    }

    ssize_t ret = pread(m_file_descriptor, buffer, n_byte, offset);

    if (ret == -1) {
        throw std::runtime_error("Error when reading file ; errno "
                                 + std::to_string(errno) + "(" + strerror(errno)
                                 + ")");
    }

    return ret;
}

size_t next_power_of_2(size_t n)
{
    // super smart and fast way to compute the smallest power of 2 greater than
    // or equal to n.
    // From
    // https://www.geeksforgeeks.org/smallest-power-of-2-greater-than-or-equal-to-n/
    // (method 4)

    n--; // to support n = 2^x
    // set to 1 all the bits after the leftmost bit set to 1
    n |= n >> 1;  // set the two leftmost bits to 1
    n |= n >> 2;  // set the 4 leftmost bits to 1
    n |= n >> 4;  // set the 8 leftmost bits to 1
    n |= n >> 8;  // set the 16 leftmost bits to 1
    n |= n >> 16; // set the 32 leftmost bits to 1
    n |= n >> 32; // set the 64 leftmost bits to 1
    n++;
    return n;
}

size_t FileBenchmark::random_aligned_read(uint8_t* buffer,
                                          size_t   n_byte,
                                          off_t*   location)
{
    // compute the alignment
    // we take the nearest power of two greater or equal than n_byte
    const size_t alignment = next_power_of_2(n_byte);

    // chose a random position
    std::uniform_int_distribution<off_t> uniform_dist(0, m_size / alignment);
    off_t offset = uniform_dist(m_random_generator) * alignment;

    if (location != nullptr) {
        *location = offset;
    }

    ssize_t ret = pread(m_file_descriptor, buffer, n_byte, offset);

    if (ret == -1) {
        throw std::runtime_error("Error when reading file ; errno "
                                 + std::to_string(errno) + "(" + strerror(errno)
                                 + ")");
    }

    return ret;
}
