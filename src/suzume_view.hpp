#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include "mustache.hpp"

struct suzume_view : public mustache::page_base {
    enum { RECENTS, BODY };

    explicit suzume_view (std::string const& a) : srcname (a) {}

    bool render (std::vector<std::string>& doc, std::string& output)
    {
        m_doc = &doc;
        std::string src;
        if (! slurp (src))
            return false;
        mustache::layout_type layout;
        layout.bind ("recents", RECENTS, mustache::FOR);
        layout.bind ("body",    BODY,    mustache::STRING);
        if (! layout.assemble (src))
            return false;
        layout.expand (*this, output);
        return true;
    }

    void iter (int symbol)
    {
        if (RECENTS == symbol) m_index = 0;
    }

    void valueof (int symbol, bool& v)
    {
        if (RECENTS == symbol) v = m_index < m_doc->size ();
    }

    void next (int symbol)
    {
        if (RECENTS == symbol) ++m_index;
    }

    void valueof (int symbol, std::string& v)
    {
        if (BODY == symbol) v = m_doc->at (m_index);
    }

private:
    std::string const srcname;
    std::vector<std::string>* m_doc;
    int m_index;

    bool slurp (std::string& src)
    {
        std::ifstream is (srcname, std::ifstream::binary);
        if (! is)
            return false;
        is.seekg (0, is.end);
        std::size_t n = is.tellg ();
        is.seekg (0, is.beg);
        src.resize (n, ' ');
        is.read (&src[0], n);
        is.close ();
        return true;
    }
};
