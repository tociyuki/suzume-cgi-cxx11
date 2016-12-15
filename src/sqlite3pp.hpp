#pragma once

#include <string>
#include <memory>
#include <utility>
#include <sqlite3.h>
#include "encode-utf8.hpp"

namespace sqlite3pp {

class statement {
private:
    std::shared_ptr<struct sqlite3_stmt> mstmt;
public:
    statement (sqlite3_stmt* pstmt) : mstmt (pstmt, sqlite3_finalize) { }
    int reset () { return sqlite3_reset (mstmt.get ()); }
    int bind (int n, int v) { return sqlite3_bind_int (mstmt.get (), n, v); }
    int bind (int n, double v) { return sqlite3_bind_double (mstmt.get (), n, v); }
    int step () { return sqlite3_step (mstmt.get ()); }
    int column_int (int n) { return sqlite3_column_int (mstmt.get (), n); }
    double column_double (int n) { return sqlite3_column_double (mstmt.get (), n); }

    int bind (int n, std::wstring s)
    {
        std::string t;
        wjson::encode_utf8 (s, t);
        return sqlite3_bind_text (
            mstmt.get (), n, t.c_str (), (int)t.size (), SQLITE_TRANSIENT);
    }

    std::wstring column_string (int n)
    {
        std::wstring t;
        std::string s ((char const*)sqlite3_column_text (mstmt.get (), n),
                       sqlite3_column_bytes (mstmt.get (), n));
        wjson::decode_utf8 (s, t);
        return t;
    }

    void column_string (int n, std::wstring& t)
    {
        std::string s ((char const*)sqlite3_column_text (mstmt.get (), n),
                       sqlite3_column_bytes (mstmt.get (), n));
        wjson::decode_utf8 (s, t);
    }
};

class connection {
private:
    std::shared_ptr<struct sqlite3> mdb;
    int mstatus;
public:
    connection (std::string s) {
        sqlite3* pdb;
        mstatus = sqlite3_open (s.c_str (), &pdb);
        mdb = std::shared_ptr<struct sqlite3>(pdb, sqlite3_close);
    }
    statement prepare (std::string s)
    {
        sqlite3_stmt* stmt;
        mstatus = sqlite3_prepare_v2 (mdb.get (), s.c_str (), s.size (), &stmt, 0);
        return statement (stmt);
    }
    int status () { return mstatus; }
    std::string errmsg () { return std::string (sqlite3_errmsg (mdb.get ())); }
    sqlite3_int64 last_insert_rowid () { return sqlite3_last_insert_rowid (mdb.get ()); }
    int execute (std::string s)
    {
        sqlite3_stmt* stmt;
        mstatus = sqlite3_prepare_v2 (mdb.get (), s.c_str (), s.size (), &stmt, 0);
        if (SQLITE_OK == mstatus)
            mstatus = sqlite3_step (stmt);
        if (SQLITE_DONE != mstatus && SQLITE_ROW != mstatus)
            std::cerr << "sqlite3_execute: " << errmsg () << ": " << s << std::endl;
        sqlite3_finalize (stmt);
        return mstatus;
    }
};

} // namespace sqlite3pp
