#pragma once

#include <cstdint>

#include <string>
#include <vector>

namespace sse {
namespace insecure {

class Index
{
public:
    using keyword_type  = std::string;
    using document_type = uint64_t;

    virtual ~Index(){};

    virtual std::vector<document_type> search(
        const keyword_type& keyword) const = 0;

    virtual void insert(const keyword_type& keyword, document_type document)
        = 0;
};

} // namespace insecure
} // namespace sse