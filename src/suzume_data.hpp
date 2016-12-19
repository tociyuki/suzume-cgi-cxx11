#pragma once

#include <string>
#include <utility>
#include "sqlite3pp.hpp"

struct suzume_data {
    explicit suzume_data (std::string const& a) : dbname (a) {}

    void insert (std::string const& body) const
    {
        sqlite3pp::connection dbh (dbname);
        dbh.execute ("BEGIN;");
        auto sth = dbh.prepare ("INSERT INTO entries VALUES (NULL, ?);");
        sth.bind (1, body);
        if (sth.step () == SQLITE_DONE)
            dbh.execute ("COMMIT;");
        else
            dbh.execute ("ROLLBACK;");
    }

    void recents (std::vector<std::string>& doc) const
    {
        std::string body;
        sqlite3pp::connection dbh (dbname);
        auto sth = dbh.prepare ("SELECT body FROM entries ORDER BY id DESC LIMIT 20;");
        for (std::size_t i = 0; SQLITE_ROW == sth.step (); ++i) {
            sth.column_string (0, body);
            doc.emplace_back ("");
            std::swap (doc.back (), body);
        }
    }

private:
    std::string const dbname;
};
