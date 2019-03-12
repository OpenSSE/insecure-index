#pragma once

#include "index.hpp"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <gtest/gtest.h>

namespace sse {
namespace test {


void iterate_database(
    const std::map<std::string, std::list<insecure::Index::document_type>>& db,
    const std::function<void(const std::string&,
                             insecure::Index::document_type)>& callback);

void iterate_database_keywords(
    const std::map<std::string, std::list<insecure::Index::document_type>>& db,
    const std::function<void(const std::string&,
                             const std::list<insecure::Index::document_type>&)>&
        callback);

inline void insert_database(
    insecure::Index* index,
    const std::map<std::string, std::list<insecure::Index::document_type>>& db)
{
    iterate_database(
        db, [index](const std::string& kw, insecure::Index::document_type doc) {
            index->insert(kw, doc);
        });
}

void test_search_correctness(
    const insecure::Index* index,
    const std::map<std::string, std::list<insecure::Index::document_type>>& db);

} // namespace test
} // namespace sse