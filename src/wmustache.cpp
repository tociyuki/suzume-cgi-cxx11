#include <iostream>
#include <string>
#include <vector>
#include "wmustache.hpp"

wmustache::wmustache () : mstmt () {}
wmustache::~wmustache () {}

bool
wmustache::assemble (std::wstring const& str)
{
    std::size_t addrsect;
    std::vector<std::size_t> backpatch;
    std::wstring::const_iterator s = str.cbegin ();
    std::wstring::const_iterator const eos = str.cend ();
    int kind;
    std::wstring plain;
    std::wstring key;
    while ((kind = next_token (s, eos, plain, key)) != ENDMARK) {
        if (! plain.empty ())
            mstmt.push_back (instruction{PLAIN, plain, 0});
        switch (kind) {
        case GETHTML:
        case GETASIS:
            mstmt.push_back (instruction{kind, key, 0});
            break;
        case SECTION:
        case UNLESS:
            backpatch.push_back (mstmt.size ());
            mstmt.push_back (instruction{kind, key, 0});
            break;
        case ENDSECT:
            if (backpatch.empty ())
                return false;
            addrsect = backpatch.back ();
            backpatch.pop_back ();
            if (mstmt[addrsect].str != key)
                return false;
            mstmt.push_back (instruction{kind, key, addrsect});
            mstmt[addrsect].addr = mstmt.size ();
            break;
        default:
            break;
        }
    }
    if (! backpatch.empty ())
        return false;
    if (! plain.empty ())
        mstmt.push_back (instruction{PLAIN, plain, 0});
    return true;
}

int
wmustache::next_token (std::wstring::const_iterator& s,
    std::wstring::const_iterator const eos,
    std::wstring& plain, std::wstring& key) const
{
    plain.clear ();
    int kind = ENDMARK;
    for (;;) {
        std::wstring::const_iterator save1 = s;
        while (s < eos) {
            if (s + 1 < eos) {
                if ('{' != s[1])
                    ++s;
                else if ('{' == s[0])
                    break;
            }
            ++s;
        }
        std::wstring::const_iterator save2 = s;
        if (save1 < save2)
            plain.append (save1, save2);
        if (s == eos)
            break;
        kind = scan_markup (s, eos, key);
        if (kind != PLAIN)
            break;
        kind = ENDMARK;
        s = save2 + 2;
        plain.append (save2, s);
    }
    return kind;
}

int
wmustache::scan_markup (std::wstring::const_iterator& s,
    std::wstring::const_iterator const eos,
    std::wstring& key) const
{
    static const int tbl[10][5] = {
    //  0   {   }  \s  \w
       {0,  0,  0,  0,  0}, // (\{\s*(\w+)\s*\}|\s*(\w+)\s*)\}\}
       {0,  5,  0,  2,  3}, // S1: '{' S5 | \s S2 | \w S3
       {0,  0,  0,  2,  3}, // S2:  \s S2 | \w S3
       {0,  0,  9,  4,  3}, // S3: '}' S9 | \s S4 | \w S3
       {0,  0,  9,  4,  0}, // S4: '}' S9 | \s S4
       {0,  0,  0,  5,  6}, // S5:  \s S5 | \w S6
       {0,  0,  8,  7,  6}, // S6: '}' S8 | \s S7 | \w S6
       {0,  0,  8,  7,  0}, // S7: '}' S8 | \s S7
       {0,  0,  9,  0,  0}, // S8: '}' S9
       {0,  0, 10,  0,  0}, // S9: '}' MATCH
    };
    key.clear ();
    s += 2; // skip L"{{"
    int next_state = 1;
    int kind = GETHTML;
    std::wstring::const_iterator beginkey = s;
    std::wstring::const_iterator endkey = s;
    for (; s < eos; ++s) {
        int cls = '{' == *s ? 1 : '}' == *s ? 2
                  : ' ' == *s ? 3 : '\t' == *s ? 3 : '\n' == *s ? 3
                  : keychar (*s) ? 4 : 0;
        int prev_state = next_state;
        next_state = tbl[prev_state][cls];
        if (1 == prev_state) {
            switch (*s) {
            case '!': return skip_comment (s, eos);
            case '{': kind = GETASIS; break;
            case '&': kind = GETASIS; next_state = 2; break;
            case '#': kind = SECTION; next_state = 2; break;
            case '^': kind = UNLESS;  next_state = 2; break;
            case '/': kind = ENDSECT; next_state = 2; break;
            }
        }
        if (0 == next_state)
            break;
        if (10 == next_state) {
            key.assign (beginkey, endkey);
            ++s;
            if (ENDSECT <= kind && '\n' == *s)
                    ++s;
            return kind;
        }
        if (4 == cls && ! (3 == prev_state || 6 == prev_state))
            beginkey = s;
        if (4 != cls && (3 == prev_state || 6 == prev_state))
            endkey = s;
    }
    return PLAIN;
}

bool
wmustache::keychar (int ch) const
{
    return ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z')
         || ('0' <= ch && ch <= '9') || '_' == ch || '?' == ch || '!' == ch
         || '/' == ch || '.' == ch || '-' == ch;
}

int wmustache::skip_comment (std::wstring::const_iterator& s,
    std::wstring::const_iterator const eos) const
{
    int level = 2;
    while (level > 1) {
        if (s >= eos)
            return PLAIN;
        if ('}' == *s)
            --level;
        else if ('{' == *s)
            ++level;
        ++s;
    }
    if (s >= eos || '}' != *s)
        return PLAIN;
    ++s;
    if ('\n' == *s)
        ++s;
    return COMMENT;
}

void wmustache::render (wjson::json& param, std::wostream& out) const
{
    std::vector<wjson::json *> env;
    env.push_back (&param);
    render_section (env, out, 0, mstmt.size ());
    env.back () = nullptr;
}

void wmustache::render_section (
    std::vector<wjson::json *>& env, std::wostream& out,
    std::size_t ip, std::size_t const ip_end) const
{
    while (ip < ip_end) {
        int op = mstmt[ip].op;
        if (PLAIN == op) {
            out << mstmt[ip].str;
            ++ip;
            continue;
        }
        wjson::json it;
        bool const exists = lookup (env, mstmt[ip].str, it);
        std::size_t const ip_next = op < SECTION ? ip : mstmt[ip].addr - 1;
        std::size_t const ip_section = ip + 1;
        if (! exists || it.is<std::nullptr_t> ()) {
            if (UNLESS == op)
                render_section (env, out, ip_section, ip_next);
        }
        else if (it.is<bool> ()) {
            if (GETHTML == op || GETASIS == op)
                out << it.dump ();
            else if (UNLESS == op && ! it.as<bool> ())
                render_section (env, out, ip_section, ip_next);
            else if (SECTION == op && it.as<bool> ())
                render_section (env, out, ip_section, ip_next);
        }
        else if (it.is<int> ()) {
            if (GETHTML == op || GETASIS == op)
                out << it.dump ();
            else if (UNLESS == op && ! it.as<int> ())
                render_section (env, out, ip_section, ip_next);
            else if (SECTION == op && it.as<int> ())
                render_section (env, out, ip_section, ip_next);            
        }
        else if (it.is<double> ()) {
            if (GETHTML == op || GETASIS == op)
                out << it.dump ();
            else if (SECTION == op)
                render_section (env, out, ip_section, ip_next);            
        }
        else if (it.is<std::wstring> ()) {
            bool const is_empty = it.as<std::wstring> ().empty ();
            if (GETASIS == op)
                out << it.as<std::wstring> ();
            else if (GETHTML == op) {
                std::wstring str (it.as<std::wstring> ());
                for (wchar_t c : str)
                    switch (c) {
                    case L'&': out << L"&amp;"; break;
                    case L'<': out << L"&lt;"; break;
                    case L'>': out << L"&gt;"; break;
                    case L'"': out << L"&quot;"; break;
                    case L'\'': out << L"&#39;"; break;
                    default: out << c; break;
                    }
            }
            else if (UNLESS == op && is_empty)
                render_section (env, out, ip_section, ip_next);
            else if (SECTION == op && ! is_empty)
                render_section (env, out, ip_section, ip_next);
        }
        else if (it.is<wjson::object> ()) {
            bool const is_empty = it.as<wjson::object> ().empty ();
            if (UNLESS == op && is_empty)
                render_section (env, out, ip_section, ip_next);
            else if (SECTION == op && ! is_empty) {
                env.push_back (&it);
                render_section (env, out, ip_section, ip_next);
                env.back () = nullptr;
                env.pop_back ();
            }
        }
        else if (it.is<wjson::array> ()) {
            if (UNLESS == op && it.as<wjson::array> ().empty ())
                render_section (env, out, ip_section, ip_next);
            else if (SECTION == op) {
                for (auto& x : it.as<wjson::array> ()) {
                    env.push_back (&x);
                    render_section (env, out, ip_section, ip_next);
                    env.back () = nullptr;
                    env.pop_back ();
                }
            }
        }
        ip = ip_next + 1;
    }
}

bool wmustache::lookup (std::vector<wjson::json *>& env,
    std::wstring const& key,
    wjson::json& it) const
{
    for (int i = env.size (); i > 0; --i)
        if (env[i - 1]->is<wjson::object> ()) {
            wjson::object const& h = env[i - 1]->as<wjson::object> ();
            wjson::object::const_iterator j = h.find (key);
            if (j != h.end ()) {
                it = j->second;
                return true;
            }
        }
    return false;
}
