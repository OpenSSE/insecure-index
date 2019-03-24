#pragma once

#include <cstdint>

#include <string>
#include <vector>

namespace rocksdb {
class Slice;
} // namespace rocksdb


namespace sse {
namespace insecure {

class Index
{
public:
    using keyword_type  = std::string;
    using document_type = uint64_t;

    virtual ~Index(){};

    virtual std::vector<document_type> search(
        const keyword_type& keyword) const = 0;

    virtual void insert(const keyword_type& keyword, document_type document)
        = 0;


    static bool deserialize_document_list(
        const char*                        data,
        size_t                             data_length,
        std::vector<Index::document_type>* result);

    static bool deserialize_document_list(
        const std::string&                 data,
        std::vector<Index::document_type>* result);
    static bool deserialize_document_list(
        const rocksdb::Slice&              data,
        std::vector<Index::document_type>* result);

    // static std::string serialize_document_list(
    // const std::vector<Index::document_type> doc_list);
};

} // namespace insecure
} // namespace sse