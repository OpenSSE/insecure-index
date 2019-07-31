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

    int fd = open(filename.c_str(), flags, 0644);

    if (fd == -1) {
        std::cerr << "Error when opening file " << filename << "; errno "
                  << errno << "\n";
        throw std::runtime_error("Error when opening file " + filename
                                 + "; errno " + std::to_string(errno) + "("
                                 + strerror(errno) + ")");
    }
    m_file_descriptor = fd;


    // get the file size using stat
    struct stat st;
    fstat(m_file_descriptor, &st);
    size_t original_size = st.st_size;


    if (original_size < size) {
        std::cout << "Filling " << size - original_size << " bytes..."
                  << std::flush;
        // get to the end of the file
        lseek(m_file_descriptor, 0UL, SEEK_END);
        // fill the file with the remaining bytes
        fill(fill_byte, size - original_size);
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
    constexpr size_t kBufferSize = 1 << 20; // 1MB
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
                throw std::runtime_error("Error when writing file ; errno "
                                         + std::to_string(errno) + "("
                                         + strerror(errno) + ")");
            }
        }
    }
}

size_t FileBenchmark::random_unaligned_read(uint8_t* buffer, size_t n_byte)
{
    // chose a random position
    std::uniform_int_distribution<off_t> uniform_dist(0, m_size - n_byte);
    off_t offset = uniform_dist(m_random_generator);

    ssize_t ret = pread(m_file_descriptor, buffer, n_byte, offset);

    if (ret == -1) {
        throw std::runtime_error("Error when reading file ; errno "
                                 + std::to_string(errno) + "(" + strerror(errno)
                                 + ")");
    }

    return ret;
}
