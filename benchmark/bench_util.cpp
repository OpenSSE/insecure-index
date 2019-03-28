#include "index.hpp"
#include "logger.hpp"
#include "rocksdb_merge_multimap.hpp"
#include "rocksdb_multimap.hpp"
#include "utils.hpp"
#include "wiredtiger_multimap.hpp"
#include "zipfian_distribution.hpp"

#include <cstdlib>

#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>

// constexpr auto base_path = "bench_db/";

typedef sse::insecure::Index* CreateIndexFunc(const std::string& path);

sse::insecure::Index* create_rocksdb_multimap(const std::string& path)
{
    return new sse::insecure::RocksDBMultiMap(path);
}

sse::insecure::Index* create_rocksdb_merge_multimap(const std::string& path)
{
    return new sse::insecure::RocksDBMergeMultiMap(path);
}

sse::insecure::Index* create_wiredtiger_multimap(const std::string& path)
{
    // create the directory
    sse::utility::create_directory(path, static_cast<mode_t>(0700));

    return new sse::insecure::WiredTigerMultimap(path);
}


struct DBCreationBenchmark : public sse::Benchmark
{
    explicit DBCreationBenchmark(std::string index_type)
        : sse::Benchmark(
              "[" + std::move(index_type)
              + "] DB creation ({0} entries): {1} ms, {2} ms/entry on average")
    {
    }
};

using database_stats_type = std::unordered_map<size_t, std::atomic<size_t>>;

database_stats_type create_test_database(const std::string& base_path,
                                         const std::string& index_type,
                                         CreateIndexFunc*   index_factory,
                                         const size_t       n_keywords,
                                         const size_t       n_entries)
{
    std::string path = base_path + "/" + index_type;

    std::cerr << "[" << index_type << "] Creating the database at " << path
              << "\n";

    std::unique_ptr<sse::insecure::Index> index((*index_factory)(path));

    std::random_device rd;
    std::mt19937       gen(rd());

    sse::ZipfianDistribution<size_t, double> kw_distrib(1.2, 0, n_keywords - 1);
    std::uniform_int_distribution<sse::insecure::Index::document_type>
        doc_distrib;

    std::atomic<size_t> n_entries_processed{0};
    database_stats_type n_entries_per_kw(n_keywords);

    sse::ThroughputBenchmark<size_t> throughput_bench("[" + index_type
                                                          + "] {0} entries/s",
                                                      std::chrono::seconds(1),
                                                      n_entries_processed);

    std::cerr << "[" << index_type << "] Start the database creation...\n";

    DBCreationBenchmark entire_construction_bench(index_type);

    std::thread throughput_bench_thread = throughput_bench.run_loop_in_thread();

    for (; n_entries_processed < n_entries; n_entries_processed++) {
        size_t                              r   = kw_distrib(gen);
        sse::insecure::Index::document_type doc = doc_distrib(gen);

        index->insert(std::to_string(r), doc);
        n_entries_per_kw[r]++;
    }

    throughput_bench.stop();
    entire_construction_bench.stop(n_entries);

    throughput_bench_thread.join();

    std::cerr << "[" << index_type << "] Database creation completed!\n";

    return n_entries_per_kw;
}


void print_database_stats(const database_stats_type& stats, size_t kw_count)
{
    std::cout << "Stats of the database: \n";
    for (size_t i = 0; i < kw_count; i++) {
        auto   it  = stats.find(i);
        size_t val = 0;

        if (it != stats.end()) {
            val = it->second;
        }
        std::cout << i << ":\t\t" << val << "\n";
    }
}

void print_database_stats(const database_stats_type& stats)
{
    size_t kw_count = stats.size();
    print_database_stats(stats, kw_count);
}

int main(int argc, char* argv[])
{
    if (argc <= 4) {
        std::cerr
            << "Usage: bench_util <bench_db_path> <index_type> <n_keywords> "
               "<n_entries>\n\t<index_type> must be "
               "chosen from the following list:\n"
               "\t\tRocksDB\n "
               "\t\tRocksDBMerge\n "
               "\t\tWiredTiger\n";

        return -1;
    }

    std::string base_path(argv[1]);

    char*            arg_index_type = argv[2];
    std::string      index_type;
    CreateIndexFunc* index_factory = nullptr;

    if (strcasecmp(arg_index_type, "RocksDB") == 0) {
        index_factory = &create_rocksdb_multimap;
        index_type    = "RocksDB";
    } else if (strcasecmp(arg_index_type, "RocksDBMerge") == 0) {
        index_factory = &create_rocksdb_merge_multimap;
        index_type    = "RocksDBMerge";
    } else if (strcasecmp(arg_index_type, "WiredTiger") == 0) {
        index_factory = &create_wiredtiger_multimap;
        index_type    = "WiredTiger";
    } else {
        std::cerr << "Invalid index type. <index_type> must be "
                     "chosen from the following list:\n"
                     "\t\tRocksDB\n "
                     "\t\tRocksDBMerge\n "
                     "\t\tWiredTiger\n";
        ;
        return -1;
    }

    if (!sse::utility::is_directory(base_path)
        && !sse::utility::create_directory(base_path,
                                           static_cast<mode_t>(0700))) {
        throw std::runtime_error(std::string(base_path)
                                 + ": unable to create directory");
    }


    size_t n_keywords = atoll(argv[3]);
    size_t n_entries  = atoll(argv[4]);

    std::cerr << "Chosen index type: " << index_type << "\n";
    std::cerr << "Number of distinct keywords: " << std::to_string(n_keywords)
              << "\n";
    std::cerr << "Number of entries: " << std::to_string(n_entries) << "\n";

    sse::Benchmark::set_log_to_console();

    database_stats_type stats = create_test_database(
        base_path, index_type, index_factory, n_keywords, n_entries);

    // print_database_stats(stats);

    return 0;
}