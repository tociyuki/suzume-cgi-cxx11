#ifndef WMUSTACHE_H
#define WMUSTACHE_H

#include <string>
#include <vector>
#include <ostream>
#include "wjson.hpp"

class wmustache final {
public:
    enum {ENDMARK, GETHTML, GETASIS, SECTION, UNLESS, ENDSECT, PLAIN, COMMENT}; 

    struct instruction {
        int op;
        std::wstring str;
        int addr;
        instruction (int a, std::wstring const& b, int c)
            : op (a), str (b), addr (c) {}
    };

    wmustache () : mstmt () {}

    bool assemble (std::wstring const& str)
    {
        int addrsect, addrend;
        std::vector<int> backpatch;
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
                addrend = mstmt.size ();
                backpatch.pop_back ();
                if (mstmt[addrsect].str != key)
                    return false;
                mstmt[addrsect].addr = addrend;
                mstmt.push_back (instruction{kind, key, addrsect});
                break;
            default:
                break;
            }
        }
        if (! backpatch.empty ())
            return false;
        if (! plain.empty ())
            mstmt.push_back (instruction{PLAIN, plain, 0});
        mstmt.push_back (instruction{ENDMARK, L"", 0});
        return true;
    }

    void render (wjson::json& param, std::wostream& out)
    {
        std::vector<wjson::json *> env;
        env.push_back (&param);
        render_section (0, mstmt.size (), env, out);
        env.back () = nullptr;
    }

private:
    std::vector<instruction> mstmt;

    wmustache (wmustache const&);
    wmustache (wmustache&&);
    wmustache& operator= (wmustache const&);
    wmustache& operator= (wmustache&&);

    bool keychar (int ch)
    {
        return ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z')
             || ('0' <= ch && ch <= '9') || '_' == ch || '?' == ch || '!' == ch
             || '/' == ch || '.' == ch || '-' == ch;
    }

    int scan_markup (std::wstring::const_iterator& s,
        std::wstring::const_iterator const eos,
        std::wstring& key)
    {
        key.clear ();
        s += 2; // skip L"{{"
        int state = 2;
        int kind = PLAIN;
        int level = 0;
        std::wstring::const_iterator beginkey = s;
        std::wstring::const_iterator endkey = s;
        for (; s < eos; ++s) {
            if (2 == state) {
                kind = GETHTML;
                switch (*s) {
                case '{': kind = GETASIS; state = 7; break;
                case '&': kind = GETASIS; state = 3; break;
                case '#': kind = SECTION; state = 3; break;
                case '^': kind = UNLESS;  state = 3; break;
                case '/': kind = ENDSECT; state = 3; break;
                case '!': kind = COMMENT; level = 2; state = 12; break;
                case ' ': case '\t': case '\n': state = 3; break;
                default:
                    if (keychar (*s)) {
                        beginkey = s;
                        state = 4;
                    }
                    else
                        return PLAIN;
                }
            }
            else if (3 == state || 7 == state) {
                if (keychar (*s)) {
                    beginkey = s;
                    ++state;
                }
                else if (! (' ' == *s || '\t' == *s || '\n' == *s))
                    return PLAIN;
            }
            else if (4 == state || 8 == state) {
                endkey = s;
                if (' ' == *s || '\t' == *s || '\n' == *s)
                    ++state;
                else if ('}' == *s)
                    state += 2;
                else if (! keychar (*s))
                    return PLAIN;
            }
            else if (5 == state || 9 == state) {
                if ('}' == *s)
                    ++state;
                else if (! (' ' == *s || '\t' == *s || '\n' == *s))
                    return PLAIN;
            }
            else if (6 == state) {
                if ('}' == *s) {
                    key.assign (beginkey, endkey);
                    ++s;
                    if (s < eos && kind >= SECTION && kind <= ENDSECT && '\n' == *s)
                        ++s;
                    return kind;
                }
                else
                    return PLAIN;
            }
            else if (10 == state) {
                if ('}' == *s)
                    state = 6;
                else
                    return PLAIN;
            }
            else if (12 == state) {
                if ('}' == *s)
                    --level;
                else if ('{' == *s)
                    ++level;
                if (level == 0) {
                    ++s;
                    return COMMENT;
                }
            }
        }
        return PLAIN;
    }

    int next_token (std::wstring::const_iterator& s,
        std::wstring::const_iterator const eos,
        std::wstring& plain, std::wstring& key)
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
            kind = ENDMARK;
            if (s == eos)
                break;
            kind = scan_markup (s, eos, key);
            if (kind < PLAIN)
                break;
            if (kind == PLAIN) {
                s = save2 + 2;
                plain.append (save2, s);
            }
        }
        return kind;
    }

    void render_section (
        int const ip_begin, int const ip_end,
        std::vector<wjson::json *>& env,
        std::wostream& out)
    {
        for (int ip = ip_begin; ip < ip_end; ++ip) {
            int op = mstmt[ip].op;
            if (PLAIN == op) {
                out << mstmt[ip].str;
                continue;
            }
            wjson::json it;
            if (GETHTML <= op && op <= UNLESS)
                lookup (env, mstmt[ip].str, it);
            if (GETHTML == op || GETASIS == op) {
                if (it.is<bool> () || it.is<int> () || it.is<double> ())
                    out << it.dump ();
                else if (it.is<std::wstring> ()) {
                    if (GETASIS == op)
                        out << it.as<std::wstring> ();
                    else {
                        std::wstring t (it.as<std::wstring> ());
                        for (auto s = t.begin (); s != t.end (); ++s)
                            switch (*s) {
                            case L'&': out << L"&amp;"; break;
                            case L'<': out << L"&lt;"; break;
                            case L'>': out << L"&gt;"; break;
                            case L'"': out << L"&quot;"; break;
                            case L'\'': out << L"&#39;"; break;
                            default: out << *s; break;
                            }
                    }
                }
            }
            else if (SECTION == op || UNLESS == op) {
                bool it_true = false;
                if (it.is<bool> ())
                    it_true = it.as<bool> ();
                else if (it.is<wjson::array> ())
                    it_true = ! it.as<wjson::array> ().empty ();
                else if (! it.is<std::nullptr_t> ())
                    it_true = true;
                if (! it_true && UNLESS == op)
                    render_section (ip + 1, mstmt[ip].addr, env, out);
                else if (it_true && SECTION == op) {
                    if (it.is<wjson::array> ())
                        for (auto& x : it.as<wjson::array> ()) {
                            env.push_back (&x);
                            render_section (ip + 1, mstmt[ip].addr, env, out);
                            env.back () = nullptr;
                            env.pop_back ();
                        }
                    else if (it.is<wjson::object> ()) {
                        env.push_back (&it);
                        render_section (ip + 1, mstmt[ip].addr, env, out);
                        env.back () = nullptr;
                        env.pop_back ();
                    }
                    else
                        render_section (ip + 1, mstmt[ip].addr, env, out);
                }
                ip = mstmt[ip].addr;
            }
        }
    }

    bool lookup (std::vector<wjson::json *>& env, std::wstring const& key, wjson::json& it)
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
};
#endif
