#include <iostream>
#include <cstdlib>
#include "http.hpp"
#include "runcgi.hpp"

void runcgi (http::appl& app)
{
    http::request req (std::cin);
    http::response res;
    extern char** environ;

    for (char** p = environ; *p; ++p) {
        std::string s (*p);
        std::size_t i = s.find ("=");
        std::string k = s.substr (0, i);
        std::string v = i < s.size () ? s.substr (i + 1) : "";
        if (k == "REQUEST_METHOD")
            req.method = v;
        else if (k == "CONTENT_TYPE")
            req.content_type = v;
        else if (k == "CONTENT_LENGTH")
            req.content_length = v;
        req.env[k] = v;
    }
    if (! app.call (req, res)) {
        res.status = "500";
        res.content_type = "text/plain; charset=UTF-8";
        res.location.clear ();
        res.body = "error";
    }
    if (res.status != "200")
        std::cout << "Status: " << res.status << "\x0d\x0a";
    if (res.status == "303") {
        std::cout << "Location: " << res.location << "\x0d\x0a";
        std::cout << "\x0d\x0a";
    }
    else {
        std::cout << "Content-Type: " << res.content_type << "\x0d\x0a";
        std::cout << "Content-Length: " << res.body.size () << "\x0d\x0a";
        std::cout << "\x0d\x0a" << res.body;
    }
}

