#pragma once

#include <string>
#include <utility>
#include <memory>
#include "sqlite3pp.hpp"

struct suzume_data {
    explicit suzume_data (std::string const& a) : dbname (a), dbh (a), sth (nullptr) {}

    void insert (std::string const& body)
    {
        dbh.execute ("BEGIN;");
        auto sth = dbh.prepare ("INSERT INTO entries VALUES (NULL, ?);");
        sth.bind (1, body);
        if (SQLITE_DONE == sth.step ())
            dbh.execute ("COMMIT;");
        else
            dbh.execute ("ROLLBACK;");
    }

    void recents_iter (void)
    {
        sth = std::make_shared<sqlite3pp::statement> (
            dbh.prepare ("SELECT body FROM entries ORDER BY id DESC LIMIT 20;"));
    }

    bool recents_step (void)
    {
        if (sth == nullptr)
            return false;
        if (SQLITE_ROW == sth->step ())
            return true;
        sth = nullptr;
        return false;
    }

    void recents_body (std::string& body)
    {
        if (sth != nullptr)
            sth->column_string (0, body);
    }

private:
    std::string const dbname;
    sqlite3pp::connection dbh;
    std::shared_ptr<sqlite3pp::statement> sth;
};
