#include "index.hpp"

#include <rocksdb/slice.h>

namespace sse {
namespace insecure {

bool Index::deserialize_document_list(const char* data,
                                      size_t      data_length,
                                      std::vector<Index::document_type>* result)
{
    if (data_length == 0) {
        *result = {};
    }

    constexpr size_t elt_size = sizeof(Index::document_type);

    result->resize(data_length / elt_size);
    size_t cpy_size = result->size() * elt_size;

    memcpy(reinterpret_cast<char*>(result->data()), data, cpy_size);

    return (cpy_size == data_length);
}

bool Index::deserialize_document_list(const std::string&                 data,
                                      std::vector<Index::document_type>* result)
{
    return deserialize_document_list(data.data(), data.length(), result);
}

bool Index::deserialize_document_list(const rocksdb::Slice&              data,
                                      std::vector<Index::document_type>* result)
{
    return deserialize_document_list(data.data(), data.size(), result);
}
} // namespace insecure
} // namespace sse