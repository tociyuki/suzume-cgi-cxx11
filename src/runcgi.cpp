#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include "http.hpp"
#include "runcgi.hpp"

static void req_from_environment (http::request& req);
static void req_patch_path_info (http::request& req);
static void res_write_stdout (http::response& res);
static char const* canonical_status_code (std::string const& code);

void
runcgi (http::appl& app)
{
    http::request req;
    http::response res;
    req.input = fdopen (dup (fileno (stdin)), "rb");
    req_from_environment (req);
    req_patch_path_info (req);
    if (req.content_length.status == "400 Bad Request")
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
    char const* const canon_status = canonical_status_code (res.status);
    if (canon_status != nullptr)
        res.status = canon_status;
    FILE* out = fdopen (dup (fileno (stdout)), "wb");
    if (res.status != "200 OK")
        std::fprintf (out, "Status: %s\x0d\x0a", res.status.c_str ());
    if (res.status == "303 See Other") {
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
    if (res.status != "303 See Other")
        std::fwrite (&res.body[0], sizeof (res.body[0]), res.body.size(), out);
    fclose (out);
}

static char const*
canonical_status_code (std::string const& code)
{
    // RFC 7231 HTTP/1.1: Semantics and Content
    static const char* STATUS_CODE[] = {
        "100 Continue",                         // 00
        "101 Switching Protocols",              // 01
        "200 OK",                               // 02
        "201 Created",                          // 03
        "202 Accepted",                         // 04
        "203 Non-Authoritative Information",    // 05
        "204 No Content",                       // 06
        "205 Reset Content",                    // 07
        "300 Multiple Choices",                 // 08
        "301 Moved Permanently",                // 09
        "302 Found",                            // 0a
        "303 See Other",                        // 0b
        "305 Use Proxy",                        // 0c
        "307 Temporary Redirect",               // 0d
        "400 Bad Request",                      // 0e
        "402 Payment Required",                 // 0f
        "403 Forbidden",                        // 10
        "404 Not Found",                        // 11
        "405 Method Not Allowed",               // 12
        "406 Not Acceptable",                   // 13
        "408 Request Timeout",                  // 14
        "409 Conflict",                         // 15
        "410 Gone",                             // 16
        "411 Length Required",                  // 17
        "413 Payload Too Large",                // 18
        "414 URI Too Long",                     // 19
        "415 Unsupported Media Type",           // 1a
        "417 Expectation Failed",               // 1b
        "426 Upgrade Required",                 // 1c
        "500 Internal Server Error",            // 1d
        "501 Not Implemented",                  // 1e
        "502 Bad Gateway",                      // 1f
        "503 Service Unavailable",              // 20
        "504 Gateway Timeout",                  // 21
        "505 HTTP Version Not Supported"        // 22
    };
    static char BASE[] = {
        -1, 20, 22, 25, 5, 31, 8, 10, 16, 24, 34, 30, 42
    };
    static unsigned short RULE[] = {
        0x1d21, 0x1d31, 0x1d41, 0x1d51, 0x1d61, 0x1da5, 0x1db5, 0x1dc5,
        0x0007, 0x0107, 0x0208, 0x0308, 0x0408, 0x0508, 0x0608, 0x0708,
        0x0809, 0x0909, 0x0a09, 0x0b09, 0x1d72, 0x0c09, 0x1d83, 0x0d09,
        0x0e0a, 0x1d94, 0x0f0a, 0x100a, 0x110a, 0x120a, 0x130a, 0x1dd6,
        0x140a, 0x150a, 0x160b, 0x170b, 0x1c0c, 0x180b, 0x190b, 0x1a0b,
        0x1d00, 0x1b0b, 0x1d0d, 0x1e0d, 0x1f0d, 0x200d, 0x210d, 0x220d
    };
    static const std::size_t NRULE = sizeof (RULE) / sizeof (RULE[0]);
    if (code.size () < 3 || (code.size () > 3 && code[3] != ' '))
        return nullptr;
    unsigned int status = 1;
    int entry = 0x1d;
    for (std::size_t i = 0; i < 3; ++i) {
        int const ch = static_cast<unsigned char> (code[i]);
        if (ch < '0' || '9' < ch)
            return nullptr;
        int const x = BASE[status - 1] + ch - '0';
        if (x < 0 || NRULE <= x || (RULE[x] & 0x000fU) != status)
            return nullptr;
        status = (RULE[x] & 0x00f0U) >> 4;
        entry = (RULE[x] & 0xff00U) >> 8;
    }
    return STATUS_CODE[entry];
}
