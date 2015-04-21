#ifndef SUZUME_DATA_H
#define SUZUME_DATA_H

#include <string>
#include "wjson.hpp"
#include "sqlite3pp.hpp"

struct suzume_data {
    suzume_data (std::string const& a) : dbname (a) {}

    void insert (std::wstring const& body) const
    {
        sqlite3pp::connection dbh (dbname);
        auto sth = dbh.prepare ("INSERT INTO entries VALUES (NULL, ?);");
        sth.bind (1, body);
        sth.step ();
    }

    void recents (wjson::json& doc) const
    {
        doc.as<wjson::object> ()[L"recents"] = wjson::json (wjson::array {});
        sqlite3pp::connection dbh (dbname);
        auto sth = dbh.prepare ("SELECT body FROM entries ORDER BY id DESC LIMIT 20;");
        while (SQLITE_ROW == sth.step ()) {
            wjson::json entry (wjson::object {});
            entry.as<wjson::object> ()[L"body"] = wjson::json (sth.column_string (0));
            doc[L"recents"].as<wjson::array> ().push_back (std::move (entry));
        }
    }

private:
    std::string const dbname;
};

#endif
