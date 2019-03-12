#include "utility.hpp"

#include <string>

#include <gtest/gtest.h>

namespace sse {
namespace test {

void iterate_database(
    const std::map<std::string, std::list<insecure::Index::document_type>>& db,
    const std::function<void(const std::string&,
                             insecure::Index::document_type)>& callback)
{
    for (auto it = db.begin(); it != db.end(); ++it) {
        const std::string& kw   = it->first;
        const auto&        list = it->second;
        for (auto index : list) {
            callback(kw, index);
        }
    }
}

void iterate_database_keywords(
    const std::map<std::string, std::list<uint64_t>>& db,
    const std::function<void(const std::string&, const std::list<uint64_t>&)>&
        callback)
{
    for (auto it = db.begin(); it != db.end(); ++it) {
        callback(it->first, it->second);
    }
}

void test_search_correctness(
    const insecure::Index* index,
    const std::map<std::string, std::list<insecure::Index::document_type>>& db)

{
    auto test_callback
        = [index](
              const std::string&                               kw,
              const std::list<insecure::Index::document_type>& expected_list) {
              const auto res_list = index->search(kw);
              const std::set<insecure::Index::document_type> res_set(
                  res_list.begin(), res_list.end());
              const std::set<insecure::Index::document_type> expected_set(
                  expected_list.begin(), expected_list.end());

              EXPECT_EQ(res_set, expected_set);
          };
    iterate_database_keywords(db, test_callback);
}

} // namespace test
} // namespace sse