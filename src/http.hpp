#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <map>

namespace http {

struct request {
    std::map<std::string,std::string> env;
    std::string method;
    std::string content_type;
    std::string content_length;
    FILE* input;
    request (FILE* ainput)
        : env (), method (), content_type (), content_length (), input (ainput) {}
};

struct formdata {
    formdata () : boundary (), parameter (), query_parameter () {}
    bool ismultipart (std::string const& content_type);
    bool decode (FILE* in, std::size_t content_length);
    bool decode_query_string (std::string const& query_string);
    std::string boundary;
    std::vector<std::wstring> parameter;
    std::vector<std::wstring> query_parameter;
};

struct response {
    std::vector<std::string> headers;
    std::string status;
    std::string content_type;
    std::string location;
    std::string body;
    response () : headers (), status ("200"),
        content_type ("text/html; charset=utf-8"),
        location (), body () {}
};

struct appl {
    appl () {}
    virtual ~appl () {}
    virtual bool call (http::request& req, http::response& res) { return false; }
};

}//namespace http
