#ifndef SUZUME_VIEW_H
#define SUZUME_VIEW_H

#include <string>
#include <fstream>
#include "wjson.hpp"
#include "wmustache.hpp"
#include "encodeu8.hpp"

struct suzume_view {
    suzume_view (std::string const& a) : srcname (a) {}

    bool render (wjson::json& doc, std::wostream& output)
    {
        std::string octets;
        if (! slurp (octets))
            return false;
        std::wstring src (decode_utf8 (octets));

        wmustache v;
        if (! v.assemble (src))
            return false;

        v.render (doc, output);
        return true;
    }

private:
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
#endif

