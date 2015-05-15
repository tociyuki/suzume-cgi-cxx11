#pragma once

/* mustache template
 * see http://mustache.github.io/mustache.5.html
 *
 *      {{ key }}       html escape expand
 *      {{{ key }}}     raw expand
 *      {{& key }}      raw expand
 *      {{# key}}block{{/ key}}  for or if
 *      {{^ key}}block{{/ key}}  unless
 *      {{! comment out }}
 *
 *      key : [\w?!/.-]+
 *
 *      {{> filename}}  not implemented (partial)
 *      {{=<% %>=}}     not implemented (change delimiters)
 */

#include <string>
#include <vector>
#include <ostream>
#include "wjson.hpp"

class wmustache {
public:
    enum {ENDMARK, GETHTML, GETASIS, SECTION, UNLESS, ENDSECT, PLAIN, COMMENT}; 

    struct instruction {
        int op;
        std::wstring str;
        int addr;
        instruction (int a, std::wstring const& b, int c)
            : op (a), str (b), addr (c) {}
    };

    wmustache ();
    virtual ~wmustache ();
    bool assemble (std::wstring const& str);
    void render (wjson::json& param, std::wostream& out) const;

protected:
    std::vector<instruction> mstmt;

private:
    wmustache (wmustache const&);
    wmustache (wmustache&&);
    wmustache& operator= (wmustache const&);
    wmustache& operator= (wmustache&&);

    int next_token (std::wstring::const_iterator& s,
        std::wstring::const_iterator const eos,
        std::wstring& plain, std::wstring& key) const;
    int scan_markup (std::wstring::const_iterator& s,
        std::wstring::const_iterator const eos,
        std::wstring& key) const;
    bool keychar (int ch) const;
    int skip_comment (std::wstring::const_iterator& s,
        std::wstring::const_iterator const eos) const;

    void render_section (
        int const ip_begin, int const ip_end,
        std::vector<wjson::json *>& env,
        std::wostream& out) const;
    bool lookup (std::vector<wjson::json *>& env,
        std::wstring const& key,
        wjson::json& it) const;
};

