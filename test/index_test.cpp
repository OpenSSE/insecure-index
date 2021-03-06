

#include "index.hpp"

#include "rocksdb_merge_multimap.hpp"
#include "rocksdb_multimap.hpp"
#include "std_multimap.hpp"
#include "utility.hpp"
#include "utils.hpp"
#include "wiredtiger_multimap.hpp"

#include <algorithm>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

namespace sse {
typedef sse::insecure::Index* CreateIndexFunc(const std::string& path);

sse::insecure::Index* create_std_multimap(const std::string& path)
{
    (void)path;
    return new sse::insecure::StdMultiMap();
}

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
    utility::create_directory(path, static_cast<mode_t>(0700));
    return new sse::insecure::WiredTigerMultimap(path);
}

class IndexTest
    : public ::testing::TestWithParam<std::pair<CreateIndexFunc*, std::string>>
{
public:
    void SetUp() override
    {
        CreateIndexFunc* factory = GetParam().first;
        index_.reset((*factory)(GetParam().second));
    }
    void TearDown() override
    {
        index_.reset(nullptr);
        utility::remove_directory(GetParam().second);
    }

protected:
    std::unique_ptr<sse::insecure::Index> index_;
};

TEST_P(IndexTest, basic_insertion)
{
    const std::map<std::string, std::list<uint64_t>> test_db
        = {{"kw_1", {0, 1}}, {"kw_2", {0}}, {"kw_3", {0}}};

    sse::test::insert_database(index_.get(), test_db);

    sse::test::test_search_correctness(index_.get(), test_db);
}

struct IndexPrintToStringParamName
{
    template<class ParamType>
    std::string operator()(const testing::TestParamInfo<ParamType>& info) const
    {
        return (info.param.second);
    }
};

INSTANTIATE_TEST_SUITE_P(
    BasicInstantiation,
    IndexTest,
    ::testing::Values(
        std::make_pair(&create_std_multimap, "StdMultimap"),
        std::make_pair(&create_rocksdb_multimap, "RocksDBMultimap"),
        std::make_pair(&create_rocksdb_merge_multimap, "RocksDBMergeMultimap"),
        std::make_pair(&create_wiredtiger_multimap, "WiredTigerMultimap")),
    IndexPrintToStringParamName());
} // namespace sse