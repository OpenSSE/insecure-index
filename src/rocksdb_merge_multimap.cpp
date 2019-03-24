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


class ResultListMergeOperator : public rocksdb::AssociativeMergeOperator
{
public:
    // virtual bool Merge(const rocksdb::Slice& key,
    //                    const rocksdb::Slice* existing_value,
    //                    const rocksdb::Slice& value,
    //                    std::string*          new_value,
    //                    rocksdb::Logger*      logger) const override
    // {
    //     // assuming 0 if no existing value
    //     std::vector<Index::document_type> existing_vec{};

    //     if (existing_value) {
    //         if (!Index::deserialize_document_list(*existing_value,
    //                                               &existing_vec)) {
    //             // if existing_value is corrupted, treat it as 0
    //             rocksdb::Log(logger, "existing value corruption");
    //             std::cerr << "corruption\n";
    //             existing_vec = {};
    //         }
    //     }

    //     std::vector<Index::document_type> deserialized_vec;
    //     if (!Index::deserialize_document_list(value, &deserialized_vec)) {
    //         // if operand is corrupted, treat it as 0
    //         Log(logger, "operand value corruption");
    //         std::cerr << "corruption\n";
    //         deserialized_vec = {};
    //     }
    //     size_t n_existing = deserialized_vec.size();

    //     deserialized_vec.resize(deserialized_vec.size() +
    //     existing_vec.size()); std::copy(existing_vec.begin(),
    //               existing_vec.end(),
    //               deserialized_vec.begin() + n_existing);
    //     // deserialized_vec.insert(
    //     // deserialized_vec.end(), existing_vec.begin(), existing_vec.end());


    //     *new_value = std::string(
    //         reinterpret_cast<char*>(deserialized_vec.data()),
    //         deserialized_vec.size() * sizeof(Index::document_type));
    //     return true; // always return true for this, since we treat all
    //     errors
    //                  // as "zero".
    // }


    // virtual bool Merge(const rocksdb::Slice& key,
    //                    const rocksdb::Slice* existing_value,
    //                    const rocksdb::Slice& value,
    //                    std::string*          new_value,
    //                    rocksdb::Logger*      logger) const override
    // {
    //     if (!existing_value) {
    //         *new_value = std::string(value.data(), value.size());
    //         return true;
    //     }

    //     size_t concat_size = value.size();
    //     if (existing_value) {
    //         concat_size += existing_value->size();
    //     }

    //     std::vector<char> data(concat_size);
    //     std::memcpy(data.data(), value.data(), value.size());

    //     if (existing_value) {
    //         std::memcpy(data.data() + value.size(),
    //                     existing_value->data(),
    //                     existing_value->size());
    //     }

    //     *new_value
    //         = std::string(reinterpret_cast<char*>(data.data()), concat_size);
    //     return true;
    // }

    // virtual bool Merge(const rocksdb::Slice& key,
    //                    const rocksdb::Slice* existing_value,
    //                    const rocksdb::Slice& value,
    //                    std::string*          new_value,
    //                    rocksdb::Logger*      logger) const override
    // {
    //     if (!existing_value) {
    //         *new_value = std::string(value.data(), value.size());
    //         return true;
    //     }
    //     std::array<char, 1024> buffer;
    //     char*                  data_ptr = nullptr;

    //     size_t concat_size = value.size();
    //     if (existing_value) {
    //         concat_size += existing_value->size();
    //     }

    //     if (concat_size > buffer.size()) {
    //         data_ptr = new char[concat_size];
    //     } else {
    //         data_ptr = buffer.data();
    //     }
    //     std::memcpy(data_ptr, value.data(), value.size());
    //     std::memcpy(data_ptr + value.size(),
    //                 existing_value->data(),
    //                 existing_value->size());

    //     *new_value = std::string(data_ptr, concat_size);

    //     if (concat_size > buffer.size()) {
    //         delete[] data_ptr;
    //     }

    //     return true;
    // }


    virtual bool Merge(const rocksdb::Slice& key,
                       const rocksdb::Slice* existing_value,
                       const rocksdb::Slice& value,
                       std::string*          new_value,
                       rocksdb::Logger*      logger) const override
    {
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

    options.write_buffer_size = 1024 * 1024; // 1MB

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