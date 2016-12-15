#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include "http.hpp"
#include "runcgi.hpp"

void runcgi (http::appl& app)
{
    FILE* in = fdopen (dup (fileno (stdin)), "rb");
    http::request req (in);
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
    fclose (in);

    FILE* out = fdopen (dup (fileno (stdout)), "wb");
    if (res.status != "200")
        std::fprintf (out, "Status: %s\x0d\x0a", res.status.c_str ());
    if (res.status == "303") {
        std::fprintf (out, "Location: %s\x0d\x0a", res.location.c_str ());
    }
    else {
        std::fprintf (out, "Content-Type: %s\x0d\x0a", res.content_type.c_str ());
        std::fprintf (out, "Content-Length: %zu\x0d\x0a", res.body.size ());
    }
    for (std::size_t i = 0; i < res.headers.size (); i += 2)
        std::fprintf (out, "%s: %s\x0d\x0a",
            res.headers[i].c_str (), res.headers[i + 1].c_str ());
    std::fprintf (out, "\x0d\x0a");
    if (res.status != "303")
        std::fwrite (&res.body[0], sizeof (res.body[0]), res.body.size(), out);
    fclose (out);
}
