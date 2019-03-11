#pragma once

#include "index.hpp"

#include <map>

namespace sse {
namespace insecure {

class StdMultiMap : public Index
{
public:
    StdMultiMap() = default;


    std::vector<Index::document_type> search(
        const Index::keyword_type& keyword) const;
    void insert(const Index::keyword_type& keyword,
                Index::document_type       document);

private:
    std::multimap<Index::keyword_type, Index::document_type> m_multimap;
};

} // namespace insecure
} // namespace sse