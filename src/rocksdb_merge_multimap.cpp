#include "rocksdb_merge_multimap.hpp"

#include "utils.hpp"

#include <rocksdb/db.h>
#include <rocksdb/memtablerep.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>

#include <iostream>

namespace sse {
namespace insecure {


class ResultListMergeOperator : public rocksdb::AssociativeMergeOperator
{
public:
    virtual bool Merge(const rocksdb::Slice& key,
                       const rocksdb::Slice* existing_value,
                       const rocksdb::Slice& value,
                       std::string*          new_value,
                       rocksdb::Logger*      logger) const override
    {
        // assuming 0 if no existing value
        std::vector<Index::document_type> existing_vec{};

        if (existing_value) {
            if (!Index::deserialize_document_list(*existing_value,
                                                  &existing_vec)) {
                // if existing_value is corrupted, treat it as 0
                rocksdb::Log(logger, "existing value corruption");
                existing_vec = {};
            }
        }

        std::vector<Index::document_type> deserialized_vec;
        if (!Index::deserialize_document_list(value, &deserialized_vec)) {
            // if operand is corrupted, treat it as 0
            Log(logger, "operand value corruption");
            deserialized_vec = {};
        }

        deserialized_vec.resize(deserialized_vec.size() + existing_vec.size());
        deserialized_vec.insert(
            deserialized_vec.end(), existing_vec.begin(), existing_vec.end());

        *new_value = std::string(
            reinterpret_cast<char*>(deserialized_vec.data()),
            deserialized_vec.size() * sizeof(Index::document_type));
        return true; // always return true for this, since we treat all errors
                     // as "zero".
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

    options.merge_operator.reset(new ResultListMergeOperator);

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
    std::string     data;
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
    // get the existing results
    std::vector<Index::document_type> previous_results = search(keyword);

    // append the new result
    previous_results.push_back(document);

    // serialize the vector
    constexpr size_t elt_size = sizeof(Index::document_type);
    rocksdb::Slice   slice(reinterpret_cast<const char*>(&document), elt_size);

    rocksdb::Status s = db_->Merge(rocksdb::WriteOptions(), keyword, slice);

    if (!s.ok()) {
        std::cerr << "Unable to merge pair in the database\nkeyword=" << keyword
                  << "\ndata=" + slice.ToString(true) + "\nRocksdb status: "
                  << s.ToString();
    }
}


} // namespace insecure
} // namespace sse