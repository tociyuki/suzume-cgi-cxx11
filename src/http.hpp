#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <vector>

namespace http {

struct request {
    std::string method;
    std::string content_type;
    std::string content_length;
    std::istream& input;
    request (std::istream& input)
        : method (), content_type (), content_length (), input (input) {}
};

struct response {
    std::string status;
    std::string content_type;
    std::string location;
    std::string body;
    response () : status ("200"),
        content_type ("text/html; charset=utf-8"),
        location (), body () {}
};

struct appl {
    appl () {}
    virtual ~appl () {}
    virtual bool call (http::request& req, http::response& res) { return false; }
};

void push_pair (
    int const state,
    std::string& name, std::string& value,
    std::vector<std::wstring>& param);

bool decode_urlencoded (
    std::istream& input, std::size_t contentlength,
    std::vector<std::wstring>& param);

bool decode_urlencoded (
    std::string const& str, std::vector<std::wstring>& param);

}//namespace http

#endif
