#ifndef SUZUME_DATA_H
#define SUZUME_DATA_H

#include <string>
#include "value.hpp"
#include "sqlite3pp.hpp"

struct suzume_data {
    suzume_data (std::string const& a) : dbname (a) {}

    void insert (std::wstring const& body) const
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

    void recents (wjson::value_type& doc) const
    {
        doc = wjson::table ();
        doc[L"recents"] = wjson::array ();
        sqlite3pp::connection dbh (dbname);
        auto sth = dbh.prepare ("SELECT body FROM entries ORDER BY id DESC LIMIT 20;");
        for (std::size_t i = 0; SQLITE_ROW == sth.step (); ++i) {
            doc[L"recents"][i] = wjson::table ();
            doc[L"recents"][i][L"body"] = sth.column_string (0);
        }
    }

private:
    std::string const dbname;
};

#endif
