#include <iostream>
#include <cstdlib>
#include "http.hpp"
#include "runcgi.hpp"

void runcgi (http::appl& app)
{
    http::request req (std::cin);
    http::response res;

    req.method = std::getenv ("REQUEST_METHOD");
    char const* ct = std::getenv ("CONTENT_TYPE");
    if (ct != nullptr)
        req.content_type = ct;
    char const* cl = std::getenv ("CONTENT_LENGTH");
    if (cl != nullptr)
        req.content_length = cl;
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

