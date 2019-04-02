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
#include <vector>

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

using database_stats_type        = std::vector<size_t>;
using database_atomic_stats_type = std::vector<std::atomic<size_t>>;

database_stats_type create_test_database(const std::string& base_path,
                                         const std::string& index_type,
                                         CreateIndexFunc*   index_factory,
                                         const size_t       n_keywords,
                                         const size_t       n_entries,
                                         const size_t       thread_count)
{
    if (thread_count < 1) {
        throw std::invalid_argument("thread_count must be >= 1");
    }

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
    // database_stats_type n_entries_per_kw(n_keywords);

    sse::ThroughputBenchmark<size_t> throughput_bench(
        "[" + index_type + "] {2} entries/s, progress: {4} \%",
        std::chrono::seconds(1),
        n_entries_processed,
        n_entries);

    std::cerr << "[" << index_type << "] Start the database creation...\n";

    DBCreationBenchmark entire_construction_bench(index_type);

    std::thread throughput_bench_thread = throughput_bench.run_loop_in_thread();

    std::vector<database_atomic_stats_type> n_entries_per_kw_vec(thread_count);
    for (size_t i = 0; i < thread_count; i++) {
        n_entries_per_kw_vec[i] = database_atomic_stats_type(n_keywords);
        for (size_t j = 0; j < n_keywords; j++) {
            n_entries_per_kw_vec[i][j] = 0;
        }
    }

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    // n_entries_per_kw_vec.reserve(thread_count);

    // launch the jobs
    for (size_t i = 0; i < thread_count; i++) {
        threads.emplace_back(
            [&](size_t t_id) {
                n_entries_per_kw_vec[t_id]
                    = database_atomic_stats_type(n_keywords);
                for (; n_entries_processed < n_entries; n_entries_processed++) {
                    size_t                              r   = kw_distrib(gen);
                    sse::insecure::Index::document_type doc = doc_distrib(gen);

                    index->insert(std::to_string(r), doc);
                    n_entries_per_kw_vec[t_id][r]++;
                }
            },
            i);
    }

    for (size_t i = 0; i < thread_count; i++) {
        threads[i].join();
    }

    throughput_bench.stop();
    entire_construction_bench.stop(n_entries);

    // combine the stats
    database_stats_type ret(n_entries_per_kw_vec[0].begin(),
                            n_entries_per_kw_vec[0].end());

    for (size_t i = 1; i < thread_count; i++) {
        for (size_t j = 0; j < n_keywords; j++) {
            ret[j] += n_entries_per_kw_vec[i][j];
        }
    }

    std::cerr << "[" << index_type << "] Database creation completed!\n";

    throughput_bench_thread.join();


    return ret;
}

void search_test_database(const std::string& base_path,
                          const std::string& index_type,
                          CreateIndexFunc*   index_factory,
                          const size_t       n_keywords)
{
    std::string path = base_path + "/" + index_type;

    std::cerr << "[" << index_type << "] Loading the database at " << path
              << "\n";

    std::unique_ptr<sse::insecure::Index> index((*index_factory)(path));

    std::cerr << "[" << index_type << "] Start the search benchmark...\n";


    for (size_t i = 0; i < n_keywords; i++) {
        sse::SearchBenchmark bench(index_type);
        auto                 result = index->search(std::to_string(i));

        bench.set_count(result.size());
    }


    std::cerr << "[" << index_type << "] Search benchmark completed!\n";
}

void print_database_stats(const database_stats_type& stats, size_t kw_count)
{
    std::cout << "Stats of the database: \n";
    for (size_t i = 0; i < kw_count; i++) {
        size_t val = stats[i];

        std::cout << i << ":\t\t" << val << "\n";
    }
}

void print_database_stats(const database_stats_type& stats)
{
    size_t kw_count = stats.size();
    print_database_stats(stats, kw_count);
}


void print_usage()
{
    std::cerr << "Usage: bench_util <bench_db_path> <index_type> <action> "
                 "<options>"
                 "\n\t<index_type> must be "
                 "chosen from the following list:\n"
                 "\t\tRocksDB\n "
                 "\t\tRocksDBMerge\n "
                 "\t\tWiredTiger\n"
                 "\n\t<action> must be chosen from the following list:\n"
                 "\t\tgenerate\n "
                 "\t\tsearch\n ";
}
int main(int argc, char* argv[])
{
    // sse::Benchmark::set_log_to_console();

    if (argc <= 3) {
        print_usage();
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

    sse::Benchmark::set_benchmark_file("benchmark_" + index_type + ".log",
                                       true);

    char* action = argv[3];

    if (strcasecmp(action, "generate") == 0) {
        if (argc <= 5) {
            std::cerr << "The \"generate\" action takes two options:\n"
                         "\t\tgenerate <n_keywords> <n_entries>\n";
            return -1;
        }
        if (!sse::utility::is_directory(base_path)
            && !sse::utility::create_directory(base_path,
                                               static_cast<mode_t>(0700))) {
            throw std::runtime_error(std::string(base_path)
                                     + ": unable to create directory");
        }


        size_t n_keywords = atoll(argv[4]);
        size_t n_entries  = atoll(argv[5]);
        size_t n_threads  = 1;

        if (index_type == "RocksDBMerge") {
            n_threads = 4;
        }

        std::cerr << "Creating a new index\n";
        std::cerr << "Chosen index type: " << index_type << "\n";
        std::cerr << "Number of distinct keywords: "
                  << std::to_string(n_keywords) << "\n";
        std::cerr << "Number of entries: " << std::to_string(n_entries) << "\n";


        database_stats_type stats = create_test_database(base_path,
                                                         index_type,
                                                         index_factory,
                                                         n_keywords,
                                                         n_entries,
                                                         n_threads);

        // print_database_stats(stats);
    } else if (strcasecmp(action, "search") == 0) {
        if (argc <= 4) {
            std::cerr << "The \"search\" action takes one options:\n"
                         "\t\tsearch <n_keywords>\n";
            return -1;
        }
        std::cerr << "Search benchmark for index type: " << index_type << "\n";

        size_t n_keywords = atoll(argv[4]);

        search_test_database(base_path, index_type, index_factory, n_keywords);
    } else {
        std::cerr << "Invalid action type. <action> must be "
                     "chosen from the following list:\n"
                     "\t\tgenerate\n "
                     "\t\tsearch\n ";
        ;
        return -1;
    }
    return 0;
}