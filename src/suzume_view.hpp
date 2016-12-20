#pragma once

#include <string>
#include <fstream>
#include "suzume_data.hpp"
#include "mustache.hpp"

struct suzume_view : public mustache::page_base {
    enum { RECENTS, BODY };

    explicit suzume_view (suzume_data& a, std::string const& b)
        : data (a), srcname (b) {}

    bool render (std::string& output)
    {
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
        if (RECENTS == symbol) data.recents_iter ();
    }

    void valueof (int symbol, bool& v)
    {
        if (RECENTS == symbol) v = data.recents_step ();
    }

    void valueof (int symbol, std::string& v)
    {
        if (BODY == symbol) data.recents_body (v);
    }

private:
    suzume_data& data;
    std::string const srcname;

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
