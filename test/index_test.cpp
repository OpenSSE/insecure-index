

#include "index.hpp"

#include "std_multimap.hpp"
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
    index_->insert("a", 1);
    index_->insert("a", 2);
    index_->insert("a", 3);
    index_->insert("b", 1);
    index_->insert("c", 2);

    auto res = index_->search("a");

    std::sort(res.begin(), res.end());
    EXPECT_EQ(res, std::vector<sse::insecure::Index::document_type>({1, 2, 3}));
}

INSTANTIATE_TEST_SUITE_P(BasicInstantiation,
                         IndexTest,
                         ::testing::Values(std::make_pair(&create_std_multimap,
                                                          "toto")));
}