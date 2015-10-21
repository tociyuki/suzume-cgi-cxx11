#ifndef SUZUME_VIEW_H
#define SUZUME_VIEW_H

#include <string>
#include <fstream>
#include "value.hpp"
#include "mustache.hpp"
#include "encode-utf8.hpp"

struct suzume_view {
    suzume_view (std::string const& a) : srcname (a) {}

    bool render (wjson::value_type& doc, std::ostream& output)
    {
        std::string octets;
        if (! slurp (octets))
            return false;
        wjson::mustache v;
        if (! v.assemble (octets))
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

