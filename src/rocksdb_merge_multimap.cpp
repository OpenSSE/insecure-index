#include "rocksdb_merge_multimap.hpp"

#include "utils.hpp"

#include <rocksdb/db.h>
#include <rocksdb/memtablerep.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>

#include <iostream>
#include <thread>

namespace sse {
namespace insecure {

std::atomic<size_t> rocksdb_merge_counter_{0};

class ResultListMergeOperator : public rocksdb::AssociativeMergeOperator
{
public:
    virtual bool Merge(const rocksdb::Slice& key,
                       const rocksdb::Slice* existing_value,
                       const rocksdb::Slice& value,
                       std::string*          new_value,
                       rocksdb::Logger*      logger) const override
    {
        rocksdb_merge_counter_++;
        if (!existing_value) {
            *new_value = std::string(value.data(), value.size());
            return true;
        }

        size_t concat_size = value.size() + existing_value->size();

        *new_value = std::string();
        new_value->reserve(concat_size); // only one memory allocation here
        new_value->append(value.data(), value.size());
        new_value->append(existing_value->data(), existing_value->size());

        return true;
    }

    virtual const char* Name() const override
    {
        return "ResultListMergeOperator";
    }
};

RocksDBMergeMultiMap::RocksDBMergeMultiMap(const std::string& path)
{
    rocksdb::Options options;
    options.create_if_missing = true;

    options.allow_concurrent_memtable_write
        = options.memtable_factory->IsInsertConcurrentlySupported();

    options.IncreaseParallelism(std::thread::hardware_concurrency());

    options.write_buffer_size       = 32 * 1024 * 1024; // 16MB
    options.max_write_buffer_number = 4;

    options.merge_operator.reset(new ResultListMergeOperator);

    options.allow_mmap_reads  = true;
    options.allow_mmap_writes = true;

    options.compression            = rocksdb::kNoCompression;
    options.bottommost_compression = rocksdb::kDisableCompressionOption;

    rocksdb::DB*    database;
    rocksdb::Status status = rocksdb::DB::Open(options, path, &database);

    if (!status.ok()) {
        std::cerr << "Unable to open the database:\n " << status.ToString();
        db_.reset(nullptr);
    } else {
        db_.reset(database);
    }
}

std::vector<Index::document_type> RocksDBMergeMultiMap::search(
    const Index::keyword_type& keyword) const
{
    std::string data;

    // set the locality counter to 0
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), keyword, &data);

    if (s.ok()) {
        std::vector<Index::document_type> result;
        constexpr size_t elt_size = sizeof(Index::document_type);

        result.resize(data.length() / elt_size);
        memcpy(reinterpret_cast<char*>(result.data()),
               data.data(),
               result.size() * elt_size);

        return result;
    }
    return {};
}

void RocksDBMergeMultiMap::insert(const Index::keyword_type& keyword,
                                  Index::document_type       document)
{
    // serialize the vector
    constexpr size_t elt_size = sizeof(Index::document_type);
    rocksdb::Slice   slice(reinterpret_cast<const char*>(&document), elt_size);

    rocksdb::Status s = db_->Merge(rocksdb::WriteOptions(), keyword, slice);

    if (!s.ok()) {
        std::cerr << "Unable to merge pair in the database\nkeyword=" << keyword
                  << "\ndata=" + slice.ToString(true) + "\nRocksdb status: "
                  << s.ToString() << "\n";
    }
}


} // namespace insecure
} // namespace sse