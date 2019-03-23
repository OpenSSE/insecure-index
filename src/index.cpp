#include "index.hpp"

#include <rocksdb/slice.h>

namespace sse {
namespace insecure {

bool Index::deserialize_document_list(const std::string&                 data,
                                      std::vector<Index::document_type>* result)
{
    if (data.length() == 0) {
        *result = {};
    }

    constexpr size_t elt_size = sizeof(Index::document_type);

    result->resize(data.length() / elt_size);
    size_t cpy_size = result->size() * elt_size;

    memcpy(reinterpret_cast<char*>(result->data()), data.data(), cpy_size);

    return (cpy_size == data.length());
}

bool Index::deserialize_document_list(const rocksdb::Slice&              data,
                                      std::vector<Index::document_type>* result)
{
    if (data.size() == 0) {
        *result = {};
    }

    constexpr size_t elt_size = sizeof(Index::document_type);

    result->resize(data.size() / elt_size);
    size_t cpy_size = result->size() * elt_size;

    memcpy(reinterpret_cast<char*>(result->data()), data.data(), cpy_size);

    return (cpy_size == data.size());
}
} // namespace insecure
} // namespace sse