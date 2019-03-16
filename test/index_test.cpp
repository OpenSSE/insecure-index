

#include "index.hpp"

#include "rocksdb_multimap.hpp"
#include "std_multimap.hpp"
#include "utility.hpp"
#include "utils.hpp"

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
    (void)path;
    return new sse::insecure::RocksDBMultiMap(path);
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
    ::testing::Values(std::make_pair(&create_std_multimap, "StdMultimap"),
                      std::make_pair(&create_rocksdb_multimap,
                                     "RocksDBMultimap")),
    IndexPrintToStringParamName());
} // namespace sse