

#include "index.hpp"

#include "std_multimap.hpp"

#include <algorithm>
#include <memory>

#include <gtest/gtest.h>

typedef sse::insecure::Index* CreateIndexFunc();

sse::insecure::Index* create_std_multimap()
{
    return new sse::insecure::StdMultiMap();
}

class IndexTest : public ::testing::TestWithParam<CreateIndexFunc*>
{
public:
    void SetUp() override
    {
        index_.reset((*GetParam())());
    }
    void TearDown() override
    {
        index_.reset(nullptr);
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
                         ::testing::Values(&create_std_multimap));