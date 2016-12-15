#include <iostream>
#include <string>
#include <vector>
#include "mustache.hpp"
#include "encode-utf8.hpp"

namespace wjson {

mustache::mustache () : m_program () {}
mustache::~mustache () {}

bool
mustache::assemble (std::string const& octets)
{
    m_program.clear ();
    if (! decode_utf8 (octets, m_source))
        return false;
    m_program.push_back({L'#', 0, 0, 0});
    std::size_t dot = 0;
    std::size_t const eos = m_source.size ();
    span_type plain {L'+', 0, 0, 0};
    span_type tag;
    std::vector<std::size_t> stack;
    while (dot < eos) {
        std::size_t const pos = dot;
        dot = match (pos, tag);
        if (pos + 1 == dot)
            continue;
        if (plain.first < pos) {
            plain.last = pos;
            m_program.push_back (plain);
        }
        plain.first = dot;
        if (L'#' == tag.code || L'^' == tag.code)
            stack.push_back (m_program.size ());
        m_program.push_back (tag);
        if (L'/' == tag.code) {
            if (stack.empty ())
                return false;
            std::size_t const loc = stack.back ();
            stack.pop_back ();
            span_type& sharp = m_program[loc];
            if (m_source.compare (sharp.first, sharp.last - sharp.first,
                    m_source, tag.first, tag.last - tag.first) != 0)
                return false;
            sharp.size = m_program.size () - loc - 2;
            m_program.back ().size = sharp.size;
        }
    }
    if (plain.first < dot) {
        plain.last = dot;
        m_program.push_back (plain);
    }
    if (! stack.empty ())
        return false;
    m_program.push_back({L'/', 0, 0, 0});
    m_program[0].size = m_program.size () - 2;
    m_program.back ().size = m_program[0].size;
    return true;
}

std::size_t
mustache::match (std::size_t const pos, span_type& op) const
{
    static const std::string CODE =
        "@@@@@@@@@DD@@D@@@@@@@@@@@@@@@@@@D@@BC@B@CCCCCCCBCCCCCCCCCCC@@C@C"
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCC@CBC@CCCCCCCCCCCCCCCCCCCCCCCCCCA@EC@";
    static const char BASE[] = {-1, 0, 1, 4, 6, 8, 10, 13, 15, 17, 18, 19};
    static const unsigned short RULE[] = {
        0x032, 0x043, 0x194, 0x164, 0x274, 0x054, 0x165, 0x275,
        0x055, 0x276, 0x066, 0x077, 0x387, 0x3d7, 0x088, 0x0d8,
        0x2a9, 0x099, 0x0aa, 0x3ba, 0x3ca, 0x0bb, 0x0cb, 0x0dc,
        0x01d,
    };
    static const std::size_t NRULE = sizeof (RULE) / sizeof (RULE[0]);
    std::size_t dot = pos;
    int next_state = 2;
    op.code = L'$';
    op.size = 0;
    for (; next_state > 1 && dot < m_source.size (); ++dot) {
        wchar_t const ch = m_source[dot];
        if (4 == next_state && L'!' == ch)
            return skip_comment (pos, op);
        int const code = ch < 127 ? CODE[ch] - '@' : 0;
        int const state = next_state;
        int const i = BASE[state - 2] + code;
        unsigned int rule = 0 <= i && i < NRULE ? RULE[i] : 0;
        next_state = (rule & 0x00fU) == state ? (rule & 0x0f0U) >> 4 : 0;
        switch (rule & 0xf00U) {
        case 0x100U:
            op.code = L'{' == ch ? L'&' : ch;
            break;
        case 0x200U:
            op.first = dot;
            break;
        case 0x300U:
            op.last = dot;
            break;
        }
    }
    if (1 != next_state)
        return pos + 1;
    if (L'$' != op.code && L'&' != op.code && dot < m_source.size ()
            && L'\n' == m_source[dot])
        ++dot;
    return dot;
}

std::size_t
mustache::skip_comment (std::size_t const pos, span_type& op) const
{
    op.code = L'!';
    op.first = pos + 3;
    int level = 2;
    std::size_t dot = pos + 3; // skip "{{!"
    std::size_t const eos = m_source.size ();
    while (level > 1 && dot < eos) {
        if (L'}' == m_source[dot])
            --level;
        else if (L'{' == m_source[dot])
            ++level;
        ++dot;
    }
    if (dot >= eos || L'}' != m_source[dot++])
        return pos + 1;
    op.last = dot - 2;
    return dot;
}

void
mustache::render (wjson::value_type& param, std::ostream& output) const
{
    std::vector<wjson::value_type *> env;
    env.push_back (&param);
    render_block (0, env, output);
    env.back () = nullptr;
}

void
mustache::render_block (std::size_t ip,
    std::vector<wjson::value_type *>& env, std::ostream& output) const
{
    std::wstring::const_iterator const s = m_source.cbegin ();
    std::wstring key;
    std::size_t const limit = ip + m_program[ip].size + 1;
    ++ip;
    for (; ip < limit; ++ip) {
        span_type const& op = m_program[ip];
        if (L'+' == op.code) {
            for (std::size_t i = op.first; i < op.last; ++i)
                encode_utf8 (output, static_cast<std::uint32_t> (m_source[i]));
        }
        else if (L'!' == op.code) {
            ;
        }
        else {
            key.assign (s + op.first, s + op.last);
            wjson::value_type it;
            bool const exists = lookup (env, key, it);
            if (! exists || it.tag () == wjson::VALUE_NULL) {
                if (L'^' == op.code)
                    render_block (ip, env, output);
            }
            else if (it.tag () == wjson::VALUE_BOOLEAN) {
                if (L'^' == op.code && ! it.boolean ())
                    render_block (ip, env, output);
                else if (L'#' == op.code && it.boolean ())
                    render_block (ip, env, output);
            }
            else if (it.tag () == wjson::VALUE_FIXNUM) {
                if (L'$' == op.code || L'&' == op.code)
                    output << it.fixnum ();
            }
            else if (it.tag () == wjson::VALUE_FLONUM) {
                if (L'$' == op.code || L'&' == op.code)
                    render_flonum (it.flonum (), output);
            }
            else if (it.tag () == wjson::VALUE_DATETIME) {
                if (L'&' == op.code)
                    render_string (it.datetime (), output);
                else if (L'$' == op.code)
                    render_html (it.datetime (), output);
                else if (L'#' == op.code)
                    render_block (ip, env, output);
            }
            else if (it.tag () == wjson::VALUE_STRING) {
                bool const is_empty = it.size () == 0;
                if (L'&' == op.code)
                    render_string (it.string (), output);
                else if (L'$' == op.code)
                    render_html (it.string (), output);
                else if (L'^' == op.code && is_empty)
                    render_block (ip, env, output);
                else if (L'#' == op.code && ! is_empty)
                    render_block (ip, env, output);
            }
            else if (it.tag () == wjson::VALUE_TABLE) {
                bool const is_empty = it.size () == 0;
                if (L'^' == op.code && is_empty) {
                    render_block (ip, env, output);
                }
                else if (L'#' == op.code && ! is_empty) {
                    env.push_back (&it);
                    render_block (ip, env, output);
                    env.back () = nullptr;
                    env.pop_back ();
                }
            }
            else if (it.tag () == wjson::VALUE_ARRAY) {
                if (L'^' == op.code && it.size () == 0) {
                    render_block (ip, env, output);
                }
                else if (L'#' == op.code) {
                    for (auto& x : it.array ()) {
                        env.push_back (&x);
                        render_block (ip, env, output);
                        env.back () = nullptr;
                        env.pop_back ();
                    }
                }
            }
        }
        if (L'#' == op.code || L'^' == op.code)
            ip += op.size + 1;
    }
}

bool
mustache::lookup (std::vector<wjson::value_type *>& env,
    std::wstring const& key, wjson::value_type& it) const
{
    for (int i = env.size (); i > 0; --i)
        if (env[i - 1]->tag () == wjson::VALUE_TABLE) {
            auto j = env[i - 1]->table ().find (key);
            if (j != env[i - 1]->table ().end ()) {
                it = j->second;
                return true;
            }
        }
    return false;
}

void
mustache::render_flonum (double const x, std::ostream& output) const
{
    char buf[32];
    std::snprintf (buf, sizeof (buf) / sizeof (buf[0]), "%.15g", x);
    std::string t (buf);
    if (t.find_first_of (".e") == std::string::npos)
        t += ".0";
    output << t;
}

void
mustache::render_string (std::wstring const& str, std::ostream& output) const
{
    for (std::wstring::const_iterator s = str.cbegin (); s < str.cend (); ++s) {
        uint32_t const uc = static_cast<uint32_t> (*s);
        encode_utf8 (output, uc);
    }
}

void
mustache::render_html (std::wstring const& str, std::ostream& output) const
{
    for (std::wstring::const_iterator s = str.cbegin (); s < str.cend (); ++s) {
        uint32_t const uc = static_cast<uint32_t> (*s);
        switch (uc) {
        default: encode_utf8 (output, uc); break;
        case '&': output << "&amp;"; break;
        case '<': output << "&lt;"; break;
        case '>': output << "&gt;"; break;
        case '"': output << "&quot;"; break;
        case '\'': output << "&#39;"; break;
        }
    }
}

}//namespace wjson
