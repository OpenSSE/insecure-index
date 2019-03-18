#include "rocksdb_multimap.hpp"

#include "utils.hpp"

#include <rocksdb/db.h>
#include <rocksdb/memtablerep.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>

#include <iostream>

namespace sse {
namespace insecure {

RocksDBMultiMap::RocksDBMultiMap(const std::string& path)
{
    rocksdb::Options options;
    options.create_if_missing = true;


    // rocksdb::CuckooTableOptions cuckoo_options;
    // cuckoo_options.identity_as_first_hash = false;
    // cuckoo_options.hash_table_ratio       = 0.9;


    // //        cuckoo_options.use_module_hash = false;
    // //        cuckoo_options.identity_as_first_hash = true;

    // options.table_cache_numshardbits = 4;
    // options.max_open_files           = -1;


    // options.table_factory.reset(rocksdb::NewCuckooTableFactory(cuckoo_options));

    // options.memtable_factory = std::make_shared<rocksdb::VectorRepFactory>();

    // options.compression            = rocksdb::kNoCompression;
    // options.bottommost_compression = rocksdb::kDisableCompressionOption;

    // options.compaction_style = rocksdb::kCompactionStyleLevel;
    // options.info_log_level   = rocksdb::InfoLogLevel::INFO_LEVEL;


    // //        options.max_grandparent_overlap_factor = 10;

    // options.delayed_write_rate         = 8388608;
    // options.max_background_compactions = 20;

    // //        options.disableDataSync = true;
    // options.allow_mmap_reads                       = true;
    // options.new_table_reader_for_compaction_inputs = true;

    options.allow_concurrent_memtable_write
        = options.memtable_factory->IsInsertConcurrentlySupported();

    // options.max_bytes_for_level_base            = 4294967296; // 4 GB
    // options.arena_block_size                    = 134217728;  // 128 MB
    // options.level0_file_num_compaction_trigger  = 10;
    // options.level0_slowdown_writes_trigger      = 16;
    // options.hard_pending_compaction_bytes_limit = 137438953472; // 128 GB
    // options.target_file_size_base               = 201327616;
    // options.write_buffer_size                   = 1073741824; // 1GB

    // //        options.optimize_filters_for_hits = true;

    rocksdb::DB*    database;
    rocksdb::Status status = rocksdb::DB::Open(options, path, &database);

    if (!status.ok()) {
        std::cerr << "Unable to open the database:\n " << status.ToString();
        db_.reset(nullptr);
    } else {
        db_.reset(database);
    }
}

std::vector<Index::document_type> RocksDBMultiMap::search(
    const Index::keyword_type& keyword) const
{
    std::string     data;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), keyword, &data);

    if (s.ok()) {
        std::vector<Index::document_type> results;
        Index::deserialize_document_list(data, &results);
        return results;
    }
    return {};
}

void RocksDBMultiMap::insert(const Index::keyword_type& keyword,
                             Index::document_type       document)
{
    // get the existing results
    std::vector<Index::document_type> previous_results = search(keyword);

    // append the new result
    previous_results.push_back(document);

    // serialize the vector
    constexpr size_t elt_size = sizeof(Index::document_type);
    rocksdb::Slice slice(reinterpret_cast<const char*>(previous_results.data()),
                         previous_results.size() * elt_size);

    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), keyword, slice);

    if (!s.ok()) {
        std::cerr << "Unable to insert pair in the database\nkeyword="
                  << keyword
                  << "\ndata=" + slice.ToString(true) + "\nRocksdb status: "
                  << s.ToString();
    }
}


} // namespace insecure
} // namespace sse