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
#include <map>

namespace mustache {

enum {
    STRING = 1, STRITER, INTEGER, DOUBLE, EXPAND
};

class layout_type;

struct span_type {
    int code;  // L'+' plain, L'$' variable, L'&' unescape, and so on.
    int symbol;
    int element;
    std::size_t size;
    std::size_t first;
    std::size_t last;
};

struct binding_type {
    int symbol;
    int element;
};

class page_base {
public:
    virtual ~page_base () {}
    virtual void valueof (int symbol, std::string& v) { v = ""; }
    virtual void valueof (int symbol, std::string::const_iterator& v1, std::string::const_iterator& v2) {}
    virtual void valueof (int symbol, long& v) { v = 0; }
    virtual void valueof (int symbol, double& v) { v = 0.0; }
    virtual void expand (layout_type const* layout, std::size_t ip, span_type const& op, std::string& output) {}
    static void append_html (int escape_level, std::string::const_iterator first, std::string::const_iterator last, std::string& output);
    static void append_html (int escape_level, double x, std::string& output);
};

class layout_type {
public:
    layout_type ();
    virtual ~layout_type ();
    void bind (std::string const& name, int symbol, int element);
    bool assemble (std::string const& str);
    void expand (page_base& page, std::string& output) const;
    void expand_block (std::size_t ip, page_base& page, std::string& output) const;

protected:
    std::size_t match (std::size_t const pos, span_type& op) const;
    std::size_t skip_comment (std::size_t const pos, span_type& op) const;

    std::string m_source;
    std::vector<span_type> m_program;
    std::map<std::string,binding_type> m_binding;

private:
    layout_type (layout_type const&);
    layout_type (layout_type&&);
    layout_type& operator= (layout_type const&);
    layout_type& operator= (layout_type&&);
};

}//namespace mustache
