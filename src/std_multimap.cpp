#include "std_multimap.hpp"

namespace sse {
namespace insecure {

std::vector<Index::document_type> StdMultiMap::search(
    const Index::keyword_type& keyword) const
{
    auto range = m_multimap.equal_range(keyword);

    std::vector<Index::document_type> result;

    result.reserve(m_multimap.count(keyword));

    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}


void StdMultiMap::insert(const Index::keyword_type& keyword,
                         Index::document_type       document)
{
    m_multimap.insert(std::make_pair(keyword, document));
}


} // namespace insecure
} // namespace sse
