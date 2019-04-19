#pragma once


#include "index.hpp"

#include <memory>

namespace rocksdb {
class DB;
}

namespace sse {
namespace insecure {

extern std::atomic<size_t> rocksdb_merge_counter_;

class RocksDBMergeMultiMap : public Index
{
public:
    RocksDBMergeMultiMap(const std::string& path);

    std::vector<Index::document_type> search(
        const Index::keyword_type& keyword) const;
    void insert(const Index::keyword_type& keyword,
                Index::document_type       document);

private:
    std::unique_ptr<rocksdb::DB> db_;
};
} // namespace insecure
} // namespace sse