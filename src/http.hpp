#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <map>

namespace http {

struct content_length_type {
    std::string string;
    std::string status;
    bool canonlength (std::string const& str);
    bool lt (std::size_t limit) const;
    std::size_t to_size (void) const;
};

struct request {
    std::map<std::string,std::string> env;
    std::string method;
    std::string content_type;
    content_length_type content_length;
    FILE* input;
    request ()
        : env (), method (), content_type (), content_length (), input (nullptr) {}
};

struct formdata {
    formdata () : boundary (), parameter (), query_parameter () {}
    bool ismultipart (std::string const& content_type);
    bool decode (FILE* in, std::size_t content_length);
    bool decode_query_string (std::string const& query_string);
    std::string boundary;
    std::vector<std::string> parameter;
    std::vector<std::string> query_parameter;
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
