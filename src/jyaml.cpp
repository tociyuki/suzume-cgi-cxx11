#include <string>
#include <utility>
#include "jyaml.hpp"
#include <iostream>

namespace wjson {

enum {CTX_BLOCK_IN, CTX_BLOCK_OUT, CTX_FLOW_OUT, CTX_FLOW_IN,
      CTX_BLOCK_KEY, CTX_FLOW_KEY};

class derivs_t {
public:
    derivs_t (std::wstring::const_iterator const bos, std::wstring::const_iterator const eos);
    ~derivs_t () {}
    derivs_t (derivs_t const& x);
    derivs_t& operator=(derivs_t const& x);
    bool scan (std::wstring const& pattern);
    bool scan_indent (int const n1, int const n2);
    bool lookahead (std::wstring const& pattern);
    bool check (std::wstring const& pattern);
    bool check_indent (int const n1, int const n2);
    bool check_bol ();
    bool check_eos ();
    int peek () const { return pbegin < peos ? *pbegin : -1; }
    int get ();
    bool match () { pbegin = pend; return true; }
    bool match (derivs_t const& x) { pbegin = pend = x.pend; return true; }
    bool fail () { pend = pbegin; return false; }
    std::wstring::const_iterator cbegin () const { return pbegin; }
    std::wstring::const_iterator cend () const { return pend; }
private:
    std::wstring::const_iterator pbos;
    std::wstring::const_iterator pbegin;
    std::wstring::const_iterator pend;
    std::wstring::const_iterator peos;
};

static int c7toi (int const c);
static bool l_document (derivs_t& s, json& value);
static bool c_forbidden (derivs_t s);
static bool c_directive (derivs_t& s0);
static bool s_l_block_node (derivs_t& s0, int const n, int const ctx, json& value);
static bool s_l_block_indented (derivs_t& s0, int const n, int const ctx, json& value);
static bool l_block_sequence (derivs_t& s0, int const n, json& value);
static bool ns_l_compact_sequence (derivs_t& s0, int const n, json& value);
static bool l_block_seq_entries (derivs_t& s0, int const n, json& value);
static bool l_block_mapping (derivs_t& s0, int const n, json& value);
static bool ns_l_compact_mapping (derivs_t& s0, int const n, json& value);
static bool ns_l_block_map_entry (derivs_t& s0, int const n, json& value);
static bool ns_flow_node (derivs_t& s0, int const n, int const ctx, json& value);
static bool ns_flow_scalar (derivs_t& s0, int const n, int const ctx, json& value);
static bool c_flow_sequence (derivs_t& s0, int const n, int const ctx0, json& value);
static bool c_flow_mapping (derivs_t& s0, int const n, int const ctx0, json& value);
static bool c_l_scalar (derivs_t& s0, int const n0, json& value);
static bool ns_plain (derivs_t& s0, int const n, int const ctx, json& value);
static bool c_quoted (derivs_t& s0, int const n, int const ctx, json& value);
static bool c_escaped (derivs_t& s, int const n, int const ctx, std::wstring& lit);
static bool s_flow_folded (derivs_t& s0, int const n, int& nbreak);
static bool c_b_block_header (derivs_t& s0, int const n, int& m, int& t);
static bool l_trail_comments (derivs_t& s0, int const n);
static bool s_separate (derivs_t& s0, int const n, int const ctx);
static bool s_b_comment (derivs_t& s0);
static bool s_l_comment (derivs_t& s0);

static int c7toi (int c)
{
    static const int tbl[128] = {
    //                                      \t  \n
        50, 50, 50, 50, 50, 50, 50, 50, 50, 39, 40, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    //       !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
        39, 37, 37, 37, 36, 37, 37, 37, 36, 36, 37, 36, 38, 36, 36, 36,
    //   0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 36, 36, 36, 36, 37, 36,
    //   @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
        37, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    //   P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 38, 36, 38, 36, 36,
    //   `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
        36, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    //   p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 38, 37, 38, 36, 50,
    };
    return 0 <= c && c <= 126 ? tbl[c] : 36;
}

std::wstring::size_type load_yaml (std::wstring const& input, json& value)
{
    wjson::derivs_t s (input.cbegin (), input.cend ());
    bool ok = l_document (s, value);
    return ok ? s.cend () - input.cbegin () : std::wstring::npos;
}

static bool l_document (derivs_t& s0, json& value)
{
    derivs_t s = s0;
    s_l_comment (s);
    while (s.scan (L"^%.%.%.%b")) {
        if (! s_l_comment (s))
            return s0.fail ();
    }
    int ndirective = 0;
    while (c_directive (s))
        ++ndirective;
    bool dirend = false;
    if (s.scan (L"---%b"))
        dirend = true;
    if (ndirective > 0 && ! dirend)
        return s0.fail ();
    if (! s_l_block_node (s, -1, CTX_BLOCK_IN, value)) {
        if (! s_l_comment (s))
            return s0.fail ();
        value = json ();
    }
    if (! s.check_eos () && ! c_forbidden (s))
        return s0.fail ();
    return s.match ();
}

static bool c_forbidden (derivs_t s)
{
    return s.check (L"^---%b") || s.check (L"^%.%.%.%b");
}

static bool c_directive (derivs_t& s0)
{
    derivs_t s = s0;
    if (s.scan (L"%%.{0,*}") && s_l_comment (s))
        return s0.match (s);
    return s0.fail ();
}

static bool s_l_block_node (derivs_t& s0, int const n, int const ctx, json& value)
{
    derivs_t s = s0;
    if (s_separate (s, n + 1, ctx) && c_l_scalar (s, n, value))
        return s0.match (s);
    s = s0;
    int n1 = CTX_BLOCK_OUT == ctx ? n - 1 : n;
    if (s_l_comment (s)
            && (l_block_sequence(s, n1, value) || l_block_mapping (s, n, value)))
        return s0.match (s);
    s = s0;
    if (s_separate (s, n + 1, CTX_FLOW_OUT)
            && ns_flow_node (s, n + 1, CTX_FLOW_OUT, value)
            && s_l_comment (s))
        return s0.match (s);
    value = json ();
    return s0.fail ();
}

static bool s_l_block_indented (derivs_t& s0, int const n, int const ctx, json& value)
{
    derivs_t s = s0;
    s.scan_indent (0, -1);
    int m = s.cend () - s0.cbegin ();
    if (ns_l_compact_sequence (s, n + 1 + m, value)
            || ns_l_compact_mapping (s, n + 1 + m, value))
        return s0.match (s);
    s = s0;
    if (s_l_block_node (s, n , ctx, value))
        return s0.match (s);
    s = s0;
    value = json ();
    if (s_l_comment (s))
        return s0.match (s);
    s = s0;
    return s0.fail ();
}

static bool l_block_sequence (derivs_t& s0, int const n, json& value)
{
    derivs_t s = s0;
    if (! s.scan_indent (n + 1, -1))
        return s0.fail ();
    int n1 = s.cend () - s0.cbegin ();
    if (! s.scan (L"-%b"))
        return s0.fail ();
    value = json (json::array {});
    return l_block_seq_entries (s0, n1, value);
}

static bool ns_l_compact_sequence (derivs_t& s0, int const n, json& value)
{
    derivs_t s = s0;
    if (! s.scan (L"-%b"))
        return s0.fail ();
    value = json (json::array {});
    json item;
    if (! s_l_block_indented (s, n, CTX_BLOCK_IN, item))
        return s0.fail ();
    value.as<array> ().push_back (item);
    s0 = s;
    return l_block_seq_entries (s0, n, value);
}

static bool l_block_seq_entries (derivs_t& s0, int const n, json& value)
{
    derivs_t s = s0;
    for (;;) {
        json item;
        if (! (s.scan_indent (n, n) && s.scan (L"-%b")))
            break;
        if (! s_l_block_indented (s, n, CTX_BLOCK_IN, item))
            break;
        value.as<array> ().push_back (std::move (item));
    }
    s0 = s;
    return true;
}

static bool l_block_mapping (derivs_t& s0, int const n, json& value)
{
    if (! s0.check_indent (n + 1, -1))
        return s0.fail ();
    int n1 = s0.cend () - s0.cbegin ();
    s0.match (s0);
    return ns_l_compact_mapping (s0, n1, value);
}

static bool ns_l_compact_mapping (derivs_t& s0, int const n, json& value)
{
    derivs_t s = s0;
    value = json (json::object {});
    if (! ns_l_block_map_entry (s, n, value))
        return s0.fail ();
    for (;;) {
        derivs_t s1 = s;
        if (! (s1.scan_indent (n, n) && ns_l_block_map_entry (s1, n, value)))
            break;
        s.match (s1);
    }
    return s0.match (s);
}

static bool ns_l_block_map_entry (derivs_t& s0, int const n, json& value)
{
    json key (L"");
    json item;
    derivs_t s = s0;
    ns_flow_scalar (s, 0, CTX_BLOCK_KEY, key);
    if (! s.scan (L"%s{0,*}:"))
        return s0.fail ();
    if (s_l_block_node (s, n, CTX_BLOCK_OUT, item)) {
        value.as<json::object> ()[key.as<std::wstring> ()] = item;
        return s0.match (s);
    }
    else if (s_l_comment (s)) {
        value.as<json::object> ()[key.as<std::wstring> ()] = json ();
        return s0.match (s);
    }
    return s0.fail ();
}

static bool ns_flow_scalar (derivs_t& s0, int const n, int const ctx, json& value)
{
    return ns_plain (s0, n, ctx, value) || c_quoted (s0, n, ctx, value);
}

static bool ns_flow_node (derivs_t& s0, int const n, int const ctx, json& value)
{
    return ns_flow_scalar (s0, n, ctx, value)
        || c_flow_sequence (s0, n, ctx, value)
        || c_flow_mapping (s0, n, ctx, value);
}

static bool c_flow_sequence (derivs_t& s0, int const n, int const ctx0, json& value)
{
    derivs_t s = s0;
    if (! s.scan (L"["))
        return s0.fail ();
    value = json (json::array {});
    s_separate (s, n, ctx0);
    int const ctx = (CTX_BLOCK_KEY == ctx0 || CTX_FLOW_KEY == ctx0 ) ? CTX_FLOW_KEY
                  : CTX_FLOW_IN;
    for (;;) {
        json item;
        if (! ns_flow_node (s, n, ctx, item))
            break;
        value.as<json::array> ().push_back (item);
        s_separate (s, n, ctx);
        if (! s.scan (L","))
            break;
        s_separate (s, n, ctx);
    }
    if (! s.scan (L"]"))
        return s0.fail ();
    return s0.match (s);
}

static bool c_flow_mapping (derivs_t& s0, int const n, int const ctx0, json& value)
{
    derivs_t s = s0;
    if (! s.scan (L"{"))
        return s0.fail ();
    value = json (json::object {});
    s_separate (s, n, ctx0);
    int const ctx = (CTX_BLOCK_KEY == ctx0 || CTX_FLOW_KEY == ctx0 ) ? CTX_FLOW_KEY
                  : CTX_FLOW_IN;
    for (;;) {
        json key (L"");
        json item;
        int got = 0;
        derivs_t la = s;
        if (ns_flow_scalar (s, n, ctx, key)) {
            s_separate (s, n, ctx);
            ++got;
        }
        if (s.scan (L":")) {
            s_separate (s, n, ctx);
            if (! s.lookahead (L",") && ! s.lookahead (L"}")
                    && ! ns_flow_node (s, n, ctx, item))
                break;
            s_separate (s, n,ctx);
            ++got;
        }
        if (got == 0)
            break;
        value.as<object> ()[key.as<std::wstring> ()] = item;
        if (! s.scan (L","))
            break;
        s_separate (s, n, ctx);
    }
    if (! s.scan (L"}"))
        return s0.fail ();
    return s0.match (s);
}

static bool c_l_scalar (derivs_t& s0, int const n0, json& value)
{
    std::wstring lit;
    derivs_t s = s0;
    int m;
    int t;
    if (! s0.lookahead (L"|") && ! s0.lookahead (L">"))
        return s0.fail ();
    int indicator = s.get ();
    if (! c_b_block_header (s, n0, m, t))
        return s0.fail ();
    int n = n0 + m;
    int nbreak = 0;
    bool clipable = false;
    bool foldable = false;
    for (;;) {
        derivs_t s1 = s;
        derivs_t s2 = s;
        derivs_t s3 = s;
        if (c_forbidden (s))
            break;
        if (s1.scan_indent (0, n) && s1.check (L"\n")) {
            s.match (s1);
            ++nbreak;
        }
        else if (s2.scan_indent (n, n) && s2.check (L"%s.{0,*}\n")) {
            if (nbreak > 0)
                lit.append (nbreak, '\n');
            lit.append (s2.cbegin (), s2.cend () - 1);
            s.match (s2);
            nbreak = 1;
            clipable = true;
            foldable = false;
        }
        else if (s3.scan_indent (n, n) && s3.check (L"%S.{0,*}\n")) {
            if (! foldable && nbreak > 0)
                lit.append (nbreak, '\n');
            else if (nbreak == 1)
                lit.push_back (' ');
            else if (nbreak > 1)
                lit.append (nbreak - 1, '\n');
            lit.append (s3.cbegin (), s3.cend () - 1);
            s.match (s3);
            nbreak = 1;
            clipable = true;
            foldable = '>' == indicator;
        }
        else
            break;
    }
    if ('+' == t && nbreak > 0)
        lit.append (nbreak, '\n');
    else if (' ' == t && clipable)
        lit.push_back ('\n');
    if (! l_trail_comments (s, n))
        return s0.fail ();
    value = json (lit);
    return s0.match (s);
}

static bool ns_plain (derivs_t& s0, int const n, int const ctx, json& value)
{
    std::wstring lit;
    derivs_t s = s0;
    if (s.check_eos () || c_forbidden (s))
        return s0.fail ();
    int ch1 = s.peek ();
    if (! s.check (L"%P"))
        return s0.fail ();
    if ('-' == ch1 || '?' == ch1 || ':' == ch1) {
        if (! s.check (L".%S"))
            return s0.fail ();
        if (CTX_FLOW_IN == ctx || CTX_FLOW_KEY == ctx) {
            if (s.check (L".%F"))
                return s0.fail ();
        }
    }
    lit.push_back (s.get ());
    for (;;) {
        derivs_t s1 = s;
        if (s1.scan (L"%s{1,*}")) {
            if ('#' == s1.peek ())
                break;
        }
        if (s1.check_eos ())
            break;
        int nbreak = -1;
        if ('\n' == s1.peek ()) {
            if (CTX_BLOCK_KEY == ctx || CTX_FLOW_KEY == ctx)
                break;
            derivs_t la = s1;
            la.get ();
            if (c_forbidden (la))
                break;
            if (! s_flow_folded (s1, n, nbreak))
                break;
        }
        if (s1.lookahead (L":%b"))
            break;
        if (CTX_FLOW_IN == ctx || CTX_FLOW_KEY == ctx) {
            if (s1.lookahead (L":%F") || s1.check (L"%F"))
                break;
        }
        if (nbreak < 0 && s.cbegin () < s1.cend ())
            lit.append (s.cbegin (), s1.cend ());
        else if (nbreak == 0)
            lit.push_back (' ');
        else if (nbreak > 0)
            lit.append (nbreak, '\n');
        s.match (s1);
        lit.push_back (s.get ());
    }
    value = json (lit);
    return s0.match (s);
}

static bool c_quoted (derivs_t& s0, int const n, int const ctx, json& value)
{
    std::wstring lit;
    derivs_t s = s0;
    if (! s.check (L"\"") && ! s.check (L"'"))
        return s0.fail ();
    int indicator = s.get ();
    std::wstring quote (1, indicator);
    for (;;) {
        if ('\'' == indicator && s.scan (L"''")) {
            lit.push_back ('\'');
            continue;
        }
        if (s.scan (quote))
            break;
        if (s.lookahead (L"%s{0,*}\n")) {
            if (CTX_BLOCK_KEY == ctx || CTX_FLOW_KEY == ctx)
                return s0.fail ();
            int nbreak = 0;
            if (s_flow_folded (s, n, nbreak)) {
                if (0 == nbreak)
                    lit.push_back (' ');
                else
                    lit.append (nbreak, '\n');
                continue;
            }
            return s0.fail ();
        }
        if (s.check (L"%s{1,*}")) {
            lit.append (s.cbegin (), s.cend ());
            s.match (s);
            continue;
        }
        int ch = s.peek ();
        if ('\t' != ch && ch < ' ')
            return s0.fail ();
        if ('\'' == indicator || '\\' != ch) {
            lit.push_back (s.get ());
            continue;
        }
        if (! c_escaped (s, n, ctx, lit))
            return s0.fail ();
    }
    value = json (lit);
    return s0.match (s);
}

static bool c_escaped (derivs_t& s, int const n, int const ctx, std::wstring& lit)
{
    s.get (); // read '\\'
    if (s.check (L"x%x%x") || s.check (L"u%x{4,4}") || s.check (L"U%x{8,8}")) {
        int x = 0;
        for (auto p = s.cbegin () + 1; p < s.cend (); ++p)
            x = x * 16 + c7toi (*p);
        lit.push_back (x);
        return s.match (s);
    }
    int esc = s.peek ();
    if ('\n' == esc) {
        if (CTX_BLOCK_KEY == ctx || CTX_FLOW_KEY == ctx)
            return s.fail ();
        int nbreak = 0;
        if (s_flow_folded (s, n, nbreak)) {
            if (nbreak > 0)
                lit.append (nbreak, '\n');
            return s.match (s);
        }
        return s.fail ();
    }
    switch (esc) {
    case '0': lit.push_back ('\0'); break;
    case 'a': lit.push_back ('\a'); break;
    case 'b': lit.push_back ('\b'); break;
    case 't': case '\t': lit.push_back ('\t'); break;
    case 'n': lit.push_back ('\n'); break;
    case 'v': lit.push_back ('\x0b'); break;
    case 'f': lit.push_back ('\f'); break;
    case 'r': lit.push_back ('\r'); break;
    case 'e': lit.push_back ('\x1b'); break;
    case 'N': lit.push_back (0x85); break;
    case '_': lit.push_back (0xa0); break;
    case 'L': lit.push_back (0x2028); break;
    case 'P': lit.push_back (0x2029); break;
    default:
        if (' ' <= esc)
            lit.push_back (esc);
        else
            return s.fail ();
    }
    s.get ();
    return s.match ();
}

static bool s_flow_folded (derivs_t& s0, int const n, int& nbreak)
{
    int const m = n < 0 ? 0 : n;
    derivs_t s = s0;
    if (! s.scan (L"%s{0,*}\n"))
        return s0.fail ();
    nbreak = 0;
    while (s.scan (L"%s{0,*}\n"))
        ++nbreak;
    if (s.check_eos ()
            || ! (s.scan_indent (m, m) && s.scan (L"%s{0,*}") && s.lookahead (L"%S")))
        return s0.fail ();
    return s0.match (s);
}

static bool c_b_block_header (derivs_t& s0, int const n, int& m, int& t)
{
    derivs_t s = s0;
    m = 1;
    t = ' ';
    int decimal = ' ';
    if (s.check (L"+") || s.check (L"-"))
        t = s.get ();
    if (s.check (L"%d"))
        decimal = s.get ();
    if (' ' == t && (s.check (L"+") || s.check (L"-")))
        t = s.get ();
    if (! s_b_comment (s))
        return s0.fail ();
    if ('0' <= decimal && decimal <= '9')
        m = decimal - '0';
    else {
        derivs_t la = s;
        while (la.scan (L"%s{0,*}\n"))
            ;
        if (la.check (L" {0,*}%S"))
            m = la.cend () - la.cbegin () - 1 - n;
    }
    if (m < 1)
        m = 1;
    return s0.match (s);
}

static bool l_trail_comments (derivs_t& s0, int const n)
{
    derivs_t s = s0;
    int const m = n < 0 ? 0 : n;
    if (! (s.scan_indent (0, m) && s.scan (L"#.{0,*}\n")))
        return s0.match (); /* l-trail-comments(n)? */
    while (! s.check_eos ())
        if (! (s.scan (L"%s{0,*}$\n{0,1}") || s.scan (L"%s{0,*}#.{0,*}$\n{0,1}")))
            break;
    return s0.match (s);
}

static bool s_separate (derivs_t& s0, int const n, int const ctx)
{
    if (CTX_BLOCK_KEY == ctx || CTX_FLOW_KEY == ctx)
        return s0.scan (L"%s{1,*}") || s0.check_bol ();
    int m = n < 0 ? 0 : n;
    derivs_t s = s0;
    if (s_l_comment (s) && s.scan_indent (m, m) && s.scan (L"%s{0,*}"))
        return s0.match (s);
    return s0.scan (L"%s{1,*}") || s0.check_bol ();
}

static bool s_b_comment (derivs_t& s0)
{
    return s0.scan (L"%s{0,*}$\n{0,1}")
         || s0.scan (L"^#.{0,*}$\n{0,1}")
         || s0.scan (L"%s{1,*}#.{0,*}$\n{0,1}");
}

static bool s_l_comment (derivs_t& s0)
{
    derivs_t s = s0;
    if (! (s_b_comment (s) || s.check_bol ()))
        return s0.fail ();
    while (! s.check_eos ())
        if (! (s.scan (L"%s{0,*}$\n{0,1}") || s.scan (L"%s{0,*}#.{0,*}$\n{0,1}")))
            break;
    return s0.match (s);
}

derivs_t::derivs_t (std::wstring::const_iterator const bos, std::wstring::const_iterator const eos)
{
    pbos = bos;
    pbegin = bos;
    pend = bos;
    peos = eos;
}

derivs_t::derivs_t (derivs_t const& x)
{
    pbos = x.pbos;
    pbegin = x.pbegin;
    pend = x.pend;
    peos = x.peos;
}

derivs_t& derivs_t::operator=(derivs_t const& x)
{
    if (this != &x) {
        pbos = x.pbos;
        pbegin = x.pbegin;
        pend = x.pend;
        peos = x.peos;
    }
    return *this;
}

int derivs_t::get ()
{
    if (pbegin >= peos)
        return -1;
    pend = ++pbegin;
    return pbegin[-1];
}

bool derivs_t::check_bol ()
{
    pend = pbegin;
    return (pbegin == pbos || '\n' == pbegin[-1]);
}

bool derivs_t::check_eos ()
{
    pend = pbegin;
    return (pbegin >= peos);
}

bool derivs_t::lookahead (std::wstring const& pattern)
{
    bool good = check (pattern);
    pend = pbegin;
    return good;
}

bool derivs_t::scan (std::wstring const& pattern)
{
    if (! check (pattern))
        return false;
    return match ();
}

bool derivs_t::scan_indent (int const n1, int const n2)
{
    if (! check_indent (n1, n2))
        return false;
    return match ();
}

bool derivs_t::check_indent (int const n1, int const n2)
{
    std::wstring::const_iterator p = pend = pbegin;
    for (int i = 0; n2 < 0 || i < n2; ++i) {
        int c = p < peos ? *p : -1;
        if (' ' == c) {
            ++p;
            continue;
        }
        if (i < n1)
            return false;
        else
            break;
    }
    pend = p;
    return true;
}

bool derivs_t::check (std::wstring const& pattern)
{
    std::wstring::const_iterator p = pend = pbegin;
    std::wstring::const_iterator ip = pattern.begin ();
    while (ip < pattern.end ()) {
        if ('^' == *ip) {
            if (p != pbos && '\n' != p[-1])
                return false;
            ++ip;
            continue;
        }
        if ('$' == *ip) {
            if (p < peos && '\n' != *p)
                return false;
            ++ip;
            continue;
        }
        int exact = *ip++;
        bool dot = ('.' == exact);
        int lower = 0; int upper = 0;
        if ('%' == exact && ip < pattern.end ()) {
            exact = *ip++;
            if ('b' == exact) {
                if (p < peos && ' ' < *p)
                    return false;
                continue;
            }
            lower = 'F' == exact ? 38 : 's' == exact ? 39 : 0;
            upper = 'd' == exact ? 10 : 'x' == exact ? 16 : 'P' == exact ? 37
                  : 'F' == exact ? 39 : 's' == exact ? 40 : 'S' == exact ? -1 : 0;
        }
        int n1 = 1; int n2 = 1;
        if (ip + 4 < pattern.end () && L'{' == *ip && L',' == ip[2] && L'}' == ip[4]) {
            n1 = c7toi (ip[1]);
            n2 = c7toi (ip[3]);
            n2 = n2 >= 36 ? -1 : n2;
            ip += 5;
        }
        for (int i = 0; n2 < 0 || i < n2; ++i) {
            int c = p < peos ? *p : -1;
            int x = c7toi (c);
            if (dot && ('\t' == c || ' ' <= c))
                ++p;
            else if (upper == -1 && 'S' == exact && ' ' < c)
                ++p;
            else if (! dot && ((upper == 0 && exact == c) || (lower <= x && x < upper)))
                ++p;
            else if (i < n1)
                return false;
            else
                break;
        }
    }
    pend = p;
    return true;
}

}//namespace wjson
