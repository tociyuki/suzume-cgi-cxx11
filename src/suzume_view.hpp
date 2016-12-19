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
        layout.bind ("recents", RECENTS, mustache::EXPAND);
        layout.bind ("body",    BODY,    mustache::STRING);
        if (! layout.assemble (src))
            return false;
        layout.expand (*this, output);
        return true;
    }

    void expand (mustache::layout_type const* layout, std::size_t ip,
        mustache::span_type const& op, std::string& output)
    {
        if (RECENTS == op.symbol) {
            if ('#' == op.code)
                for (m_index = 0; m_index < m_doc->size (); ++m_index)
                    layout->expand_block (ip, *this, output);
        }
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

    void
    append_html (std::string const& str, std::string& output)
    {
        for (std::size_t i = 0; i < str.size (); ++i) {
            int ch = static_cast<unsigned char> (str[i]);
            switch (ch) {
            case '&': output += "&amp;"; break;
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '"': output += "&quot;"; break;
            default:  output.push_back (ch);
            }
        }
    }
};
