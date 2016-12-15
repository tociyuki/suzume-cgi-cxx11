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
    struct span_type {
        wchar_t code;  // L'+' plain, L'$' variable, L'&' unescape, and so on.
        std::size_t size;
        std::size_t first;
        std::size_t last;
    };

    mustache ();
    virtual ~mustache ();
    bool assemble (std::string const& str);
    void render (wjson::value_type& param, std::ostream& output) const;

protected:
    std::wstring m_source;
    std::vector<span_type> m_program;
    std::size_t match (std::size_t const pos, span_type& op) const;
    std::size_t skip_comment (std::size_t const pos, span_type& op) const;
    void render_block (std::size_t ip, std::vector<wjson::value_type *>& env,
        std::ostream& output) const;
    bool lookup (std::vector<wjson::value_type *>& env, std::wstring const& key,
        wjson::value_type& it) const;
    void render_flonum (double const x, std::ostream& out) const;
    void render_string (std::wstring const& str, std::ostream& out) const;
    void render_html (std::wstring const& str, std::ostream& out) const;

private:
    mustache (mustache const&);
    mustache (mustache&&);
    mustache& operator= (mustache const&);
    mustache& operator= (mustache&&);
};

}//namespace wjson
