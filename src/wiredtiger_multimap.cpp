#include "wiredtiger_multimap.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace sse {
namespace insecure {

WiredTigerMultimap::WiredTigerMultimap(const std::string& path)
{
    // Open a connection to the database, creating it if necessary.
    int ret = wiredtiger_open(path.c_str(), NULL, "create", &m_wt_connection);

    if (ret != 0) {
        throw std::runtime_error("Unable to open the database. Error code: "
                                 + std::to_string(ret));
    }


    // Open a session handle for the database.
    ret = m_wt_connection->open_session(
        m_wt_connection, NULL, NULL, &m_wt_session);

    if (ret != 0) {
        throw std::runtime_error(
            "Unable to open a database session. Error code: "
            + std::to_string(ret));
    }


    // Create the table
    ret = m_wt_session->create(
        m_wt_session,
        "table:index",
        "key_format=S,value_format=u,access_pattern_hint=random");

    if (ret != 0) {
        throw std::runtime_error("Unable to create a table. Error code: "
                                 + std::to_string(ret));
    }

    // Open the cursor
    ret = m_wt_session->open_cursor(
        m_wt_session, "table:index", NULL, NULL, &m_wt_cursor);

    if (ret != 0) {
        throw std::runtime_error("Unable to open a cursor. Error code: "
                                 + std::to_string(ret));
    }
}

WiredTigerMultimap::~WiredTigerMultimap()
{
    m_wt_connection->close(m_wt_connection, NULL);

    m_wt_connection = nullptr;
    m_wt_session    = nullptr;
    m_wt_cursor     = nullptr;
}

std::vector<Index::document_type> WiredTigerMultimap::search(
    const Index::keyword_type& keyword) const
{
    m_wt_cursor->set_key(m_wt_cursor, keyword.c_str());

    int ret = m_wt_cursor->search(m_wt_cursor);

    if (ret == WT_NOTFOUND) {
        return {};
    }

    if (ret != 0) {
        throw std::runtime_error("Search: Error when searching keyword \""
                                 + keyword
                                 + "\"\ncode: " + std::to_string(ret));
    }

    WT_ITEM value;
    ret = m_wt_cursor->get_value(m_wt_cursor, &value);
    if (ret != 0) {
        throw std::runtime_error(
            "Search: Error when getting the value for keyword \"" + keyword
            + "\"\ncode: " + std::to_string(ret));
    }

    std::vector<Index::document_type> results;
    if (!Index::deserialize_document_list(
            reinterpret_cast<const char*>(value.data), value.size, &results)) {
        std::cerr << "Corruption!\n";
    }

    ret = m_wt_cursor->reset(m_wt_cursor);
    if (ret != 0) {
        std::cerr << "Search: Error when reseting the cursor for keyword \""
                  << keyword << "\"\ncode: " << std::to_string(ret) << "\n";
    }
    return results;
}
void WiredTigerMultimap::insert(const Index::keyword_type& keyword,
                                Index::document_type       document)
{
    m_wt_cursor->set_key(m_wt_cursor, keyword.c_str());

    int ret = m_wt_cursor->search(m_wt_cursor);

    if ((ret != 0) && (ret != WT_NOTFOUND)) {
        throw std::runtime_error("Insert: Error when searching keyword \""
                                 + keyword
                                 + "\"\ncode: " + std::to_string(ret));
    }

    bool                  insert_new_entry = (ret == WT_NOTFOUND);
    Index::document_type* doc_list         = nullptr;
    WT_ITEM               value;

    if (insert_new_entry) {
        value.data = &document;
        value.size = sizeof(document);
    } else {
        ret = m_wt_cursor->get_value(m_wt_cursor, &value);

        if (ret != 0) {
            std::cerr << "Insert: Error when getting the value for keyword \""
                      << keyword << "\"\ncode: " << std::to_string(ret) << "\n";
        }

        const Index::document_type* old_list
            = reinterpret_cast<const Index::document_type*>(value.data);
        Index::document_type* doc_list
            = new Index::document_type[value.size + 1];

        size_t n_elts = value.size / sizeof(document);

        std::copy(old_list, old_list + n_elts, doc_list);
        doc_list[n_elts] = document;

        value.data = doc_list;
        value.size += sizeof(document);
    }

    m_wt_cursor->set_value(m_wt_cursor, &value);
    ret = m_wt_cursor->update(m_wt_cursor);
    if (ret != 0) {
        std::cerr << "Insert: Error when updating the value for keyword \""
                  << keyword << "\"\ncode: " << std::to_string(ret) << "\n";
    }


    ret = m_wt_cursor->reset(m_wt_cursor);
    if (ret != 0) {
        std::cerr << "Insert: Error when reseting the cursor for keyword \""
                  << keyword << "\"\ncode: " << std::to_string(ret) << "\n";
    }
    delete[] doc_list;
}


} // namespace insecure
} // namespace sse
