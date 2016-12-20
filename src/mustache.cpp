#include <string>
#include <vector>
#include <cstdio>
#include "mustache.hpp"

namespace mustache {

// match XML entity: '&' ('#' ([0-9]+ | 'x' [0-9A-Fa-f]+) | [A-Za-z0-9]+) ';'
static bool
scan_entity (std::string::const_iterator& first, std::string::const_iterator last)
{
    static const char CODE[] =
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@B@@A@@@@@@@@@FFFFFFFFFF@G@@@@"
        "@CCCCCCDDDDDDDDDDDDDDDDDDDD@@@@@@CCCCCCDDDDDDDDDDDDDDDDDEDD@@@@@";
    static const char BASE[] = {0-1, 1-2, 6-3, 11-5, 14-6, 13-3};
    static const unsigned char RULE[] = {
        0x32, 0x53, 0x43, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x44, 0x14,
        0x75, 0x65, 0x77, 0x66, 0x16, 0x77, 0x17
    };
    static const std::size_t NRULE = sizeof (RULE) / sizeof (RULE[0]);
    std::string::const_iterator p = first;
    int state = 2;
    for (int count = 0; count < 32 && state > 1 && p != last; ++count) {
        int const ch = static_cast<unsigned char> (*p++);
        int const code = ch < 128 ? CODE[ch] - '@' : 0;
        int const i = BASE[state - 2] + code;
        unsigned int const rule = 0 <= i && i < NRULE ? RULE[i] : 0;
        state = (rule & 0x0fU) == state ? ((rule & 0xf0U) >> 4) : 0;
    }
    if (1 == state)
        first = p - 1;
    return 1 == state;
}

// append HTML string
// if 0==escape_level then unescape
// if 1==escape_level then escape HTML without already escaped entity
// if 2==escape_level then escape HTML everything
void
page_base::append_html (int escape_level, std::string::const_iterator first, std::string::const_iterator last, std::string& output)
{
    if (0 == escape_level) {
        output.append (first, last);
        return;
    }
    std::string::const_iterator amp;
    for (; first < last; ++first)
        switch (*first) {
        default: output.push_back (*first); break;
        case '<': output += "&lt;"; break;
        case '>': output += "&gt;"; break;
        case '"': output += "&quot;"; break;
        case '&':
            amp = first;
            if (2 == escape_level || ! scan_entity (first, last))
                output += "&amp;";
            else
                output.append (amp, first + 1);
            break;
        }
}

void
page_base::append_html (int escape_level, double x, std::string& output)
{
    char buf[32];
    std::snprintf (buf, sizeof (buf) / sizeof (buf[0]), "%.15g", x);
    std::string t = buf;
    if (t.find_first_of (".e") == t.npos)
        t += ".0";
    output += t;
}

layout_type::layout_type () : m_source (), m_program (), m_binding () {}
layout_type::~layout_type () {}

void
layout_type::bind (std::string const& name, int symbol, int element)
{
    m_binding[name] = {symbol, element};
}

void
layout_type::expand (page_base& page, std::string& output) const
{
    expand_block (0, page, output);
}

void
layout_type::expand_block (std::size_t ip, page_base& page, std::string& output) const
{
    std::string::const_iterator s = m_source.cbegin ();
    std::size_t const limit = ip + m_program[ip].size + 1;
    ++ip;
    for (; ip < limit; ++ip) {
        span_type const& op = m_program[ip];
        if ('+' == op.code) {
            output.append (s + op.first, s + op.last);
        }
        else if (STRING == op.element) {
            if ('$' == op.code || '&' == op.code) {
                std::string v;
                page.valueof (op.symbol, v);
                page_base::append_html ('$' == op.code ? 2 : 0, v.cbegin (), v.cend (), output);
            }
        }
        else if (STRITER == op.element) {
            if ('$' == op.code || '&' == op.code) {
                std::string::const_iterator v1 = m_source.cbegin ();
                std::string::const_iterator v2 = v1;
                page.valueof (op.symbol, v1, v2);
                page_base::append_html ('$' == op.code ? 2 : 0, v1, v2, output);
            }
        }
        else if (INTEGER == op.element) {
            if ('$' == op.code || '&' == op.code) {
                long v = 0;
                page.valueof (op.symbol, v);
                output += std::to_string (v);
            }
        }
        else if (DOUBLE == op.element) {
            if ('$' == op.code || '&' == op.code) {
                double v = 0.0;
                page.valueof (op.symbol, v);
                page_base::append_html (2, v, output);
            }
        }
        else if (IF == op.element) {
            if ('#' == op.code || '^' == op.code) {
                bool v = false;
                page.valueof (op.symbol, v);
                if (v ^ ('^' == op.code))
                    expand_block (ip, page, output);
            }
        }
        else if (FOR == op.element) {
            if ('#' == op.code || '^' == op.code) {
                page.iter (op.symbol);
                bool v = false;
                page.valueof (op.symbol, v);
                if ('#' == op.code)
                    while (v) {
                        expand_block (ip, page, output);
                        page.next (op.symbol);
                        page.valueof (op.symbol, v);
                    }
                else if (! v)
                    expand_block (ip, page, output);
            }
        }
        else if (CUSTOM == op.element) {
            page.expand (*this, ip, op, output);
        }
        if ('#' == op.code)
            ip += op.size + 1;
    }
}

bool
layout_type::assemble (std::string const& src)
{
    m_source = src;
    m_program.clear ();
    m_program.push_back({'#', 0, 0, 0, 0, 0});
    span_type plain {'+', 0, 0, 0, 0, 0};
    span_type tag;
    std::vector<std::size_t> section_nest;
    std::size_t dot = 0;
    std::size_t const eos = m_source.size ();
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
        if ('!' == tag.code)
            continue;
        if ('#' == tag.code || '^' == tag.code)
            section_nest.push_back (m_program.size ());
        m_program.push_back (tag);
        if ('/' == tag.code) {
            if (section_nest.empty ())
                return false;
            std::size_t const loc = section_nest.back ();
            span_type& section = m_program[loc];
            section_nest.pop_back ();
            if (m_source.compare (section.first, section.last - section.first,
                    m_source, tag.first, tag.last - tag.first) != 0)
                return false;
            section.size = m_program.back ().size = m_program.size () - loc - 2;
        }
    }
    if (plain.first < dot) {
        plain.last = dot;
        m_program.push_back (plain);
    }
    m_program.push_back({'/', 0, 0, 0, 0, 0});
    m_program[0].size = m_program.back ().size = m_program.size () - 2;
    return section_nest.empty ();
}

std::size_t
layout_type::match (std::size_t const pos, span_type& op) const
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
    op.code = '$';
    op.size = 0;
    for (; next_state > 1 && dot < m_source.size (); ++dot) {
        int const ch = static_cast<unsigned char> (m_source[dot]);
        if (4 == next_state && '!' == ch)
            return skip_comment (pos, op);
        int const code = ch < 127 ? CODE[ch] - '@' : 0;
        int const state = next_state;
        int const i = BASE[state - 2] + code;
        unsigned int rule = 0 <= i && i < NRULE ? RULE[i] : 0;
        next_state = (rule & 0x00fU) == state ? (rule & 0x0f0U) >> 4 : 0;
        switch (rule & 0xf00U) {
        case 0x100U:
            op.code = '{' == ch ? '&' : ch;
            break;
        case 0x200U:
            op.first = dot;
            break;
        case 0x300U:
            op.last = dot;
            break;
        }
    }
    if (1 == next_state) {
        auto it = m_binding.find (m_source.substr (op.first, op.last - op.first));
        if (it != m_binding.end ()) {
            op.symbol = it->second.symbol;
            op.element = it->second.element;
        }
        if ('$' != op.code && '&' != op.code
                && dot < m_source.size () && '\n' == m_source[dot])
            ++dot;
    }
    return 1 == next_state ? dot : pos + 1;
}

std::size_t
layout_type::skip_comment (std::size_t const pos, span_type& op) const
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

}//namespace mustache
