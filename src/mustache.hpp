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
#include "value.hpp"

namespace wjson {

class mustache {
public:
    enum {PLAIN, GETASIS, GETHTML, ENDSECT, UNLESS, SECTION, COMMENT, ENDMARK};

    struct instruction {
        int op;
        std::wstring str;
        std::size_t addr;
        instruction (int a, std::wstring const& b, std::size_t c)
            : op (a), str (b), addr (c) {}
    };

    mustache ();
    virtual ~mustache ();
    bool assemble (std::string const& str);
    void render (wjson::value_type& param, std::ostream& out) const;

protected:
    std::vector<instruction> mstmt;

private:
    mustache (mustache const&);
    mustache (mustache&&);
    mustache& operator= (mustache const&);
    mustache& operator= (mustache&&);

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
        std::vector<wjson::value_type *>& env, std::ostream& out,
        std::size_t ip, std::size_t const ip_end) const;
    bool lookup (std::vector<wjson::value_type *>& env,
        std::wstring const& key,
        wjson::value_type& it) const;

    void render_flonum (double const x, std::ostream& out) const;
    void render_string (std::wstring const& str, std::ostream& out) const;
    void render_html (std::wstring const& str, std::ostream& out) const;
};

}//namespace wjson
