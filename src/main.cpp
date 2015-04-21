#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "wjson.hpp"
#include "suzume_data.hpp"
#include "suzume_view.hpp"
#include "http.hpp"
#include "runcgi.hpp"

enum { POST_LIMIT = 1024 };

struct suzume_appl : public http::appl {
    suzume_data data;
    suzume_view view;

    suzume_appl (std::string const& dbname, std::string const& srcname)
        : data (dbname), view (srcname) {}

    bool get_frontpage (http::request& req, http::response& res)
    {
        wjson::json doc (wjson::object {});
        data.recents (doc);
        std::wostringstream content;
        if (! view.render (doc, content))
            return false;
        res.content_type = "text/html; charset=UTF-8";
        res.body = encode_utf8 (content.str ());
        return true;
    }

    bool post_body (std::vector<std::wstring>& param, http::request& req, http::response& res)
    {
        for (auto i = param.begin (); i < param.end (); i += 2) {
            if (i[0] == L"body") {
                data.insert (i[1]);
                res.status = "303";
                res.location = "suzume.cgi";
                return true;
            }
        }
        return false;
    }

    bool call (http::request& req, http::response& res)
    {
        if (req.method == "GET")
            return get_frontpage (req, res);
        else if (req.method == "POST") {
            long content_length = 0;
            try {
                content_length = std::stoi (req.content_length);
            }
            catch (...) {
                return false;
            }
            if (content_length > POST_LIMIT)
                return false;
            std::vector<std::wstring> param;
            http::decode_urlencoded (req.input, content_length, param);
            return post_body (param, req, res);
        }
        return false;
    }
};

int main ()
{
    suzume_appl  app ("data/suzume.db", "view/suzume.html");

    runcgi (app);

    return EXIT_SUCCESS;
}
