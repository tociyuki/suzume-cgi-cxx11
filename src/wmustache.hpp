#ifndef WMUSTACHE_H
#define WMUSTACHE_H

#include <string>
#include <vector>
#include <ostream>
#include "wjson.hpp"

class wmustache final {
public:
    enum {ENDMARK, GETHTML, GETASIS, SECTION, UNLESS, ENDSECT, PLAIN}; 

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

    int next_token (
        std::wstring::const_iterator& s,
        std::wstring::const_iterator const eos,
        std::wstring& plain,
        std::wstring& key)
    {
        plain.clear ();
        key.clear ();
        while (s < eos) {
            std::wstring::const_iterator save1 = s;
            while (s < eos) {
                if (s + 1 < eos) {
                    if (L'{' != s[1])
                        ++s;
                    else if (L'{' == s[0])
                        break;
                }
                ++s;
            }
            std::wstring::const_iterator save2 = s;
            if (save1 < save2)
                plain.append (save1, save2);
            if (s >= eos)
                break;
            s += 2;
            wchar_t mark = s < eos ? *s : L'\0';
            if (L'!' == mark) {
                ++s;
                int level = 2;
                while (s < eos && level > 0) {
                    if (L'}' == *s)
                        --level;
                    else if (L'{' == *s)
                        ++level;
                    ++s;
                }
                continue;
            }
            int kind = L'{' == mark || L'&' == mark ? GETASIS
                     : L'#' == mark ? SECTION
                     : L'^' == mark ? UNLESS
                     : L'/' == mark ? ENDSECT
                     : GETHTML;
            if (GETHTML != kind)
                ++s;
            while (s < eos && (L' ' == *s || L'\t' == *s || L'\r' == *s || L'\n' == *s))
                ++s;
            std::wstring::const_iterator save3 = s;
            while (s < eos && ((L'0' <= *s && *s <= L'9')
                    || (L'A' <= *s && *s <= L'Z') || (L'a' <= *s && *s <= L'z')
                    || L'?' == *s || L'!' == *s || L'/' == *s || L'.' == *s || L'-' == *s
                    || L'_' == *s))
                ++s;
            std::wstring::const_iterator save4 = s;
            while (s < eos && (L' ' == *s || L'\t' == *s || L'\r' == *s || L'\n' == *s))
                ++s;
            if (save3 < save4 && s + 1 < eos && L'}' == s[1] && L'}' == s[0]) {
                s += 2;
                if (L'{' == mark) {
                    if (s < eos && L'}' == *s)
                        ++s;
                    else
                        kind = PLAIN;
                }
                if (L'#' == mark || L'^' == mark || L'/' == mark) {
                    if (s < eos && L'\n' == *s)
                        ++s;
                }
            }
            else
                kind = PLAIN;
            if (kind != PLAIN) {
                key.append (save3, save4);
                return kind;
            }
            plain.append (save2, s);
        }
        return ENDMARK;
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
