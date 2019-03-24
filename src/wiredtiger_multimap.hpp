#pragma once

#include "index.hpp"

#include <wiredtiger.h>

namespace sse {
namespace insecure {


class WiredTigerMultimap : public Index
{
public:
    WiredTigerMultimap(const std::string& path);
    ~WiredTigerMultimap() override;

    std::vector<Index::document_type> search(
        const Index::keyword_type& keyword) const override;
    void insert(const Index::keyword_type& keyword,
                Index::document_type       document) override;

private:
    WT_CONNECTION* m_wt_connection{nullptr};
    WT_SESSION*    m_wt_session{nullptr};
    WT_CURSOR*     m_wt_cursor{nullptr};
};

} // namespace insecure
} // namespace sse
