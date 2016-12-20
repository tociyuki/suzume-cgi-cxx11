#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include "http.hpp"
#include "runcgi.hpp"

static void req_from_environment (http::request& req);
static void req_patch_path_info (http::request& req);
static void res_write_stdout (http::response& res);

void
runcgi (http::appl& app)
{
    http::request req;
    http::response res;
    req.input = fdopen (dup (fileno (stdin)), "rb");
    req_from_environment (req);
    req_patch_path_info (req);
    if (req.content_length.status == "400")
        res.bad_request ();
    else if (! app.call (req, res))
        res.internal_server_error ();
    fclose (req.input);
    res_write_stdout (res);
}

static void
req_from_environment (http::request& req)
{
    for (char const* const* p = environ; *p; ++p) {
        char const* eq = std::strchr (*p, '=');
        if (eq == nullptr)
            continue;
        std::string k = std::string(*p, eq - *p);
        std::string v = std::string (eq + 1);
        if (k == "REQUEST_METHOD")
            req.method = v == "POST" ? v : "GET";
        else if (k == "CONTENT_TYPE")
            req.content_type = v;
        else if (k == "CONTENT_LENGTH")
            req.content_length.canonlength (v);
        req.env[k] = v;
    }
}

static void
req_patch_path_info (http::request& req)
{
    if (req.env.count ("PATH_INFO") == 0)
        req.env["PATH_INFO"] = "";
    if (req.env.at ("SCRIPT_NAME") == "/") {
        req.env["PATH_INFO"] = req.env["SCRIPT_NAME"] + req.env["PATCH_INFO"];
        req.env["SCRIPT_NAME"] = "";
    }
}

static void
res_write_stdout (http::response& res)
{
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
