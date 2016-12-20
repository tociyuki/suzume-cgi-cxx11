#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <csignal>
#include "suzume_data.hpp"
#include "suzume_view.hpp"
#include "http.hpp"
#include "runcgi.hpp"

enum { POST_LIMIT = 1024 };

struct suzume_appl : public http::appl {
    std::string dbname;
    std::string srcname;

    suzume_appl (std::string const& adbname, std::string const& asrcname)
        : dbname (adbname), srcname (asrcname) {}

    bool get_frontpage (http::request& req, http::response& res)
    {
        suzume_data data (dbname);
        suzume_view view (data, srcname);
        res.content_type = "text/html; charset=UTF-8";
        return view.render (res.body);
    }

    bool post_body (std::vector<std::string>& param, http::request& req, http::response& res)
    {
        suzume_data data (dbname);
        for (auto it = param.begin (); it != param.end (); it += 2) {
            if (it[0] == "body") {
                data.insert (it[1]);
                res.status = "303";
                res.location = "suzume.cgi";
                return true;
            }
        }
        return false;
    }

    bool call (http::request& req, http::response& res)
    {
        if (req.method == "GET") {
            return get_frontpage (req, res);
        }
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
            http::formdata formdata;
            if (! formdata.ismultipart (req.content_type))
                return false;
            if (! formdata.decode (req.input, content_length))
                return false;
            return post_body (formdata.parameter, req, res);
        }
        return false;
    }
};

int main ()
{
    std::signal (SIGPIPE, SIG_IGN);

    suzume_appl  app ("data/suzume.db", "view/suzume.html");
    runcgi (app);

    return EXIT_SUCCESS;
}
