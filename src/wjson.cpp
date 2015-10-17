#include <utility>
#include <deque>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include "wjson.hpp"

namespace wjson {

json::json () : mguard (json::JSONNULL)
{
    mboolean = false;
}

json::json (json const& x) : mguard (x.mguard)
{
    copy_data (x);
}

json::json (json&& x) : mguard (x.mguard)
{
    move_data (std::move (x));
}

json::~json ()
{
    destroy ();
}

json::json (std::nullptr_t x) : mguard (json::JSONNULL)
{
    mboolean = false;
}

json::json (bool x) : mguard (json::BOOLEAN)
{
    mboolean = x;
}

json::json (int x) : mguard (json::INTEGER) {
    minteger = x;
}

json::json (double x) : mguard (json::NUMBER)
{
    if (std::isnan (x)) throw std::out_of_range ("json(double): nan invalid.");
    if (std::isinf (x)) throw std::out_of_range ("json(double): inf invalid.");
    mnumber = x;
}

json::json (wchar_t const* x) : mguard (json::STRING)
{
    new (&mstring) std::wstring (x);
}

json::json (std::wstring const& x) : mguard (json::STRING)
{
    new (&mstring) std::wstring (x);
}

json::json (std::wstring&& x) : mguard (json::STRING)
{
    new (&mstring) std::wstring (std::move(x));
}

json::json (array const& x) : mguard (json::ARRAY)
{
    new (&marray) array (x);
}

json::json (array&& x) : mguard (json::ARRAY)
{
    new (&marray) array (std::move(x));
}

json::json (object const& x) : mguard (json::OBJECT)
{
    new (&mobject) object (x);
}

json::json (object&& x) : mguard (json::OBJECT)
{
    new (&mobject) object (std::move(x));
}

json& json::operator=(json const& x)
{
    if (this != &x) {
        destroy ();
        mguard = x.mguard;
        copy_data (x);
    }
    return *this;
}

json& json::operator=(json&& x)
{
    if (this != &x) {
        destroy ();
        mguard = x.mguard;
        move_data (std::move (x));
    }
    return *this;
}

void json::replace (json const& x)
{
    if (this != &x) {
        destroy ();
        mguard = x.mguard;
        copy_data (x);
    }
}

void json::swap (json& x)
{
    if (mguard != x.mguard)
        throw std::logic_error ("swap only same type together");
    std::swap (mguard, x.mguard);
    switch (mguard) {
    case BOOLEAN:
        std::swap (mboolean, x.mboolean);
        break;
    case INTEGER:
        std::swap (minteger, x.minteger);
        break;
    case NUMBER:
        std::swap (mnumber, x.mnumber);
        break;
    case STRING:
        std::swap (mstring, x.mstring);
        break;
    case ARRAY:
        std::swap (marray, x.marray);
        break;
    case OBJECT:
        std::swap (mobject, x.mobject);
        break;
    default:
        break;
    }
}

std::wstring json::dump () const
{
    std::wstring out;
    dump (out);
    return out;
}

json::variation json::guard () const
{
    return mguard;
}

void json::copy_data (json const& x)
{
    switch (mguard) {
    case BOOLEAN:
        mboolean = x.mboolean;
        break;
    case INTEGER:
        minteger = x.minteger;
        break;
    case NUMBER:
        mnumber = x.mnumber;
        break;
    case STRING:
        new (&mstring) std::wstring (x.mstring);
        break;
    case ARRAY:
        new (&marray) array (x.marray);
        break;
    case OBJECT:
        new (&mobject) object (x.mobject);
        break;
    default:
        break;
    }
}

void json::move_data (json&& x)
{
    switch (mguard) {
    case STRING:
        new (&mstring) std::wstring (std::move (x.mstring));
        break;
    case ARRAY:
        new (&marray) array (std::move (x.marray));
        break;
    case OBJECT:
        new (&mobject) object (std::move (x.mobject));
        break;
    default:
        copy_data (x);
        break;
    }
}

void json::destroy ()
{
    switch (mguard) {
    case STRING:
        mstring.~basic_string ();
        break;
    case ARRAY:
        marray.~vector ();
        break;
    case OBJECT:
        mobject.~map ();
        break;
    default:
        break;
    }
}


template<> bool json::is<std::nullptr_t> () const { return mguard == JSONNULL; }
template<> bool json::is<bool> () const { return mguard == BOOLEAN; }
template<> bool json::is<int> () const { return mguard == INTEGER; }
template<> bool json::is<double> () const { return mguard == NUMBER; }
template<> bool json::is<std::wstring> () const { return mguard == STRING; }
template<> bool json::is<array> () const { return mguard == ARRAY; }
template<> bool json::is<object> () const { return mguard == OBJECT; }

template<> bool const& json::as<bool> () const
{
    if (mguard != BOOLEAN) throw std::logic_error ("as: not boolean.");
    return mboolean;
}

template<> bool& json::as<bool> ()
{
    if (mguard != BOOLEAN) throw std::logic_error ("as: not boolean.");
    return mboolean;
}

template<> int const& json::as<int> () const
{
    if (mguard != INTEGER) throw std::logic_error ("as: not integer.");
    return minteger;
}

template<> int& json::as<int> ()
{
    if (mguard != INTEGER) throw std::logic_error ("as: not integer.");
    return minteger;
}

template<> double const& json::as<double> () const
{
    if (mguard != NUMBER) throw std::logic_error ("as: not number.");
    return mnumber;
}

template<> double& json::as<double> ()
{
    if (mguard != NUMBER) throw std::logic_error ("as: not number.");
    return mnumber;
}

template<> std::wstring const& json::as<std::wstring> () const
{
    if (mguard != STRING) throw std::logic_error ("as: not string.");
    return mstring;
}

template<> std::wstring& json::as<std::wstring> ()
{
    if (mguard != STRING) throw std::logic_error ("as: not string.");
    return mstring;
}

template<> array const& json::as<array> () const
{
    if (mguard != ARRAY) throw std::logic_error ("as: not array.");
    return marray;
}

template<> array& json::as<array> ()
{
    if (mguard != ARRAY) throw std::logic_error ("as: not array.");
    return marray;
}

template<> object const& json::as<object> () const
{
    if (mguard != OBJECT) throw std::logic_error ("as: not object.");
    return mobject;
}

template<> object& json::as<object> ()
{
    if (mguard != OBJECT) throw std::logic_error ("as: not object.");
    return mobject;
}

loader::loader ()
{
}

loader::~loader ()
{
}

bool loader::isjsonspace (wchar_t const c)
{
    return L' ' == c || L'\n' == c || L'\t' == c || L'\r' == c || L'\f' == c;
}

bool loader::isjsondigit (wchar_t const c)
{
    return L'0' <= c && c <= L'9';
}

json const& json::operator[] (size_t idx) const
{
    static json nullval;
    return mguard == ARRAY && idx < marray.size () ? marray[idx] : nullval;
}

json& json::operator[] (size_t idx)
{
    static json nullval;
    return mguard == ARRAY && idx < marray.size () ? marray[idx] : nullval;
}

json const& json::operator[] (std::wstring const& key) const
{
    static json nullval;
    if (mguard != OBJECT)
        return nullval;
    object::const_iterator i = mobject.find (key);
    return i != mobject.end () ? i->second : nullval;
}

json& json::operator[] (std::wstring const& key)
{
    static json nullval;
    if (mguard != OBJECT)
        return nullval;
    object::iterator i = mobject.find (key);
    return i != mobject.end () ? i->second : nullval;
}

bool loader::parse (std::wstring const& src, json& node)
{
    enum {NSTATE = 25, LRCOL = 15, ERROR = 0, ACC = -99};
    static int const gotocol[12] = {
     // :v  :v  :v  :v  :a  :a :vl  :vl :o  :o  :ml, :ml
        10, 10, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14};
    static int const size_rhs[12] = {1, 1, 1, 1, 2, 3, 3, 1, 2, 3, 5, 3};
    static int const grammar[NSTATE][LRCOL] = {
        //    [    ]    {    }    :    ,   SC  STR    $  :v  :a :vl  :o :ml
        {0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0},
        {0,   7,   0,   8,   0,   0,   0,   3,   4,   0,  2,  5,  0,  6,  0},//1
        {0,   0,   0,   0,   0,   0,   0,   0,   0, ACC,  0,  0,  0,  0,  0},//2
        {0,   0,  -1,   0,  -1,   0,  -1,   0,   0,  -1,  0,  0,  0,  0,  0},//3
        {0,   0,  -2,   0,  -2,   0,  -2,   0,   0,  -2,  0,  0,  0,  0,  0},//4
        {0,   0,  -3,   0,  -3,   0,  -3,   0,   0,  -3,  0,  0,  0,  0,  0},//5
        {0,   0,  -4,   0,  -4,   0,  -4,   0,   0,  -4,  0,  0,  0,  0,  0},//6
        {0,   7,   9,   8,   0,   0,   0,   3,   4,   0, 11,  5, 10,  6,  0},//7
        {0,   0,   0,   0,  12,   0,   0,   0,  14,   0,  0,  0,  0,  0, 13},//8
        {0,   0,  -5,   0,  -5,   0,  -5,   0,   0,  -5,  0,  0,  0,  0,  0},//9
        {0,   0,  15,   0,   0,   0,  16,   0,   0,   0,  0,  0,  0,  0,  0},//10
        {0,   0,  -8,   0,   0,   0,  -8,   0,   0,   0,  0,  0,  0,  0,  0},//11
        {0,   0,  -9,   0,  -9,   0,  -9,   0,   0,  -9,  0,  0,  0,  0,  0},//12
        {0,   0,   0,   0,  17,   0,  18,   0,   0,   0,  0,  0,  0,  0,  0},//13
        {0,   0,   0,   0,   0,  19,   0,   0,   0,   0,  0,  0,  0,  0,  0},//14
        {0,   0,  -6,   0,  -6,   0,  -6,   0,   0,  -6,  0,  0,  0,  0,  0},//15
        {0,   7,   0,   8,   0,   0,   0,   3,   4,   0, 20,  5,  0,  6,  0},//16
        {0,   0, -10,   0, -10,   0, -10,   0,   0, -10,  0,  0,  0,  0,  0},//17
        {0,   0,   0,   0,   0,   0,   0,   0,  21,   0,  0,  0,  0,  0,  0},//18
        {0,   7,   0,   8,   0,   0,   0,   3,   4,   0, 22,  5,  0,  6,  0},//19
        {0,   0,  -7,   0,   0,   0,  -7,   0,   0,   0,  0,  0,  0,  0,  0},//20
        {0,   0,   0,   0,   0,  23,   0,   0,   0,   0,  0,  0,  0,  0,  0},//21
        {0,   0,   0,   0, -12,   0, -12,   0,   0,   0,  0,  0,  0,  0,  0},//22
        {0,   7,   0,   8,   0,   0,   0,   3,   4,   0, 24,  5,  0,  6,  0},//23
        {0,   0,   0,   0, -11,   0, -11,   0,   0,   0,  0,  0,  0,  0,  0},//24
    };
    std::deque<int> sstack {1};
    std::deque<json> dstack {json (nullptr)}; // centinel
    std::wstring::const_iterator s = src.cbegin ();
    std::wstring::const_iterator eos = src.cend ();
    json token_value;
    int token_type = next_token (s, eos, token_value);
    for (;;) {
        int ctrl = grammar[sstack.back ()][token_type];
        if (ERROR == ctrl)
            break;
        else if (ctrl > 0) {    // shift
            sstack.push_back (ctrl);
            dstack.push_back (std::move (token_value));
            token_value = json (nullptr);
            token_type = next_token (s, eos, token_value);
        }
        else if (ctrl < 0) {    // reduce or accept
            int expr = -(ctrl + 1);
            int nrhs = size_rhs[expr];
            std::deque<json>::iterator v = dstack.end () - nrhs - 1;
            json value;
            switch (ctrl) {
            //case  -1: break;  // value : SCALAR
            //case  -2: break;  // value : STRING
            //case  -3: break;  // value : array
            //case  -4: break;  // value : object
            case  -5:   // value : '[' ']'
                value = json (json::array {});
                break;
            case  -6:   // array : '[' value_list ']'
                std::swap (value, v[2]);
                break;
            case  -7:   // value_list : value_list ',' value
                std::swap (value, v[1]);
                value.as<json::array> ().push_back (std::move (v[3]));
                break;
            case  -8:   // value_list : value
                {
                    json::array a;
                    a.push_back (std::move (v[1]));
                    value = json (std::move (a));
                }
                break;
            case  -9:   // object : '{' '}'
                value = json (json::object {});
                break;
            case -10:   // object : '{' member_list '}'
                std::swap (value, v[2]);
                break;
            case -11:   // member_list : member_list ',' STRING ':' value
                std::swap (value, v[1]);
                value.as<json::object> ()[std::move(v[3].as<std::wstring> ())] = std::move (v[5]);
                break;
            case -12:   // member_list : STRING ':' value
                {
                    json::object h;
                    h[std::move (v[1].as<std::wstring> ())] = std::move (v[3]);
                    value = json (std::move (h));
                }
                break;
            case ACC:
                std::swap (node, dstack.back ());
                return true;
            default:
                if (nrhs > 0)
                    std::swap (value, v[1]);
                break;
            }
            for (int i = 0; i < nrhs; ++i)
                dstack.pop_back ();
            for (int i = 0; i < nrhs; ++i)
                sstack.pop_back ();
            int next_state = grammar[sstack.back ()][gotocol[expr]];
            sstack.push_back (next_state);
            dstack.push_back (std::move (value));
        }
    }
    return false;
}

int loader::next_token (loader::cursor& s, loader::cursor const& eos, json& node)
{
    enum {ERROR=0, BEGIN_ARRAY, END_ARRAY, BEGIN_OBJECT, END_OBJECT,
           NAME_SEPARATOR, VALUE_SEPARATOR, SCALAR, STRING, ENDMARK};
    static const std::wstring kn (L"null");
    static const std::wstring kt (L"true");
    static const std::wstring kf (L"false");
    while (s < eos && isjsonspace (*s))
        ++s;
    if (s >= eos)
        return ENDMARK;
    else if (s + kn.size () <= eos && std::equal (kn.cbegin (), kn.cend (), s)) {
        s += kn.size ();
        node = std::move (json (nullptr));
        return SCALAR;
    }
    else if (s + kt.size () <= eos && std::equal (kt.cbegin (), kt.cend (), s)) {
        s += kt.size ();
        node = std::move (json (true));
        return SCALAR;
    }
    else if (s + kf.size () <= eos && std::equal (kf.cbegin (), kf.cend (), s)) {
        s += kf.size ();
        node = std::move (json (false));
        return SCALAR;
    }
    else if (L'-' == *s || isjsondigit (*s)) {
        if (scan_number (s, eos, node))
            return SCALAR;
    }
    else if (L'"' == *s) {
        if (scan_string (s, eos, node))
            return STRING;
    }
    else switch (*s++) {
    case L'[': return BEGIN_ARRAY;
    case L']': return END_ARRAY;
    case L'{': return BEGIN_OBJECT;
    case L'}': return END_OBJECT;
    case L':': return NAME_SEPARATOR;
    case L',': return VALUE_SEPARATOR;
    default: break;
    }
    return ERROR;
}

// number <- '-'?('0'/[1-9][0-9]*)('.'[0-9]+)?([eE][+-]?[0-9]+)?
bool loader::scan_number (loader::cursor& s, loader::cursor const& eos, json& node)
{
    std::wstring::const_iterator const s0 = s;
    bool intdecimal = true;
    if (s != eos && L'-' == *s)
        ++s;
    if (s == eos || ! isjsondigit (*s))
        return false;
    if (L'0' == *s)
        ++s;
    else {
        ++s;
        while (s != eos && isjsondigit (*s))
            ++s;
    }
    if (s != eos && L'.' == *s) {
        ++s;
        if (s == eos || ! isjsondigit (*s))
            return false;
        while (s != eos && isjsondigit (*s))
            ++s;
        intdecimal = false;
    }
    if (s != eos && (L'e' == *s || L'E' == *s)) {
        ++s;
        if (s != eos && (L'-' == *s || L'+' == *s))
            ++s;
        if (s == eos || ! isjsondigit (*s))
            return false;
        while (s != eos && isjsondigit (*s))
            ++s;
        intdecimal = false;
    }
    std::wstring decimal (s0, s);
    if (intdecimal)
        try {
            int i = std::stoi (decimal);
            node = json (i);
        }
        catch (std::out_of_range) {
            intdecimal = false;
        }
    if (! intdecimal)
        try {
            double n = std::stod (decimal);
            node = json (n);
        }
        catch (std::out_of_range) {
            return false;
        }
    return true;
}

// string <- ["] char* ["]
// char <- [\x20-\x21\x23-\x5b\x5c-\x{10ffff}]
//       / '\\' (["\\/bfnrt] / 'u'[0-9a-fA-F]{4})
bool loader::scan_string (loader::cursor& s, loader::cursor const& eos, json& node)
{
    static const wchar_t U16SPHFROM = 0xd800;
    static const wchar_t U16SPHLAST = 0xdbff;
    static const wchar_t U16SPLFROM = 0xdc00;
    static const wchar_t U16SPLLAST = 0xdfff;
    static const wchar_t U16SPOFFSET = 0x35fdc00L;
    std::wstring str;
    if (s == eos || L'"' != *s)
        return false;
    ++s;
    while (s != eos && L'"' != *s) {
        wchar_t c = *s++;
        if (c < 0x20 || c > 0x10ffff)
            return false;
        if (L'\\' == c) {
            if (eos == s)
                return false;
            c = *s++;
            switch (c) {
            case 'b': c = L'\b'; break;
            case 'f': c = L'\f'; break;
            case 'n': c = L'\n'; break;
            case 'r': c = L'\r'; break;
            case 't': c = L'\t'; break;
            case '\\': break;
            case '/': break;
            case '"': break;
            case 'u':
                if (! decode_u16 (s, eos, c))
                    return false;
                // UTF-16 surrogate pair
                if (U16SPLFROM <= c && c <= U16SPLLAST)
                    return false;
                if (U16SPHFROM <= c && c <= U16SPHLAST) {
                    if (eos <= s + 1 || L'\\' != s[0] || L'u' != s[1])
                        return false;
                    s += 2;
                    wchar_t c1;
                    if (! decode_u16 (s, eos, c1))
                        return false;
                    if (c1 < U16SPLFROM || U16SPLLAST < c1)
                        return false;
                    c = (c << 10) + c1 - U16SPOFFSET;
                }
                break;
            default:
                return false;
            }
        }
        str.push_back (c);
    }
    if (s == eos || L'"' != *s)
        return false;
    ++s;
    node = json (str);
    return true;
}

bool loader::decode_u16 (loader::cursor& s, loader::cursor const& eos, wchar_t& c)
{
    wchar_t x = 0;
    for (int i = 0; i < 4; ++i, ++s) {
        if (eos == s)
            return false;
        wchar_t y = *s;
        if (L'0' <= y && y <= L'9')
            x = (x << 4) + y - L'0';
        else if (L'a' <= y && y <= L'f')
            x = (x << 4) + y - L'a' + 10;
        else if (L'A' <= y && y <= L'F')
            x = (x << 4) + y - L'A' + 10;
        else
            return false;
    }
    c = x;
    return true;
}

void json::dump (std::wstring& out) const
{
    int count = 0;
    switch (mguard) {
    case JSONNULL:
        out += L"null";
        break;
    case BOOLEAN:
        out += mboolean ? L"true" : L"false";
        break;
    case INTEGER:
        out += std::to_wstring (minteger);
        break;
    case NUMBER:
        {
            wchar_t buf[32];
            std::swprintf (buf, sizeof(buf)/sizeof(buf[0]), L"%.15g", mnumber);
            std::wstring s (buf);
            if (s.find_first_of (L".e") == std::wstring::npos)
                s += L".0";
            out += s;
        }
        break;
    case STRING:
        out += L"\"";
        for (wchar_t c : mstring) {
            if (L'\\' == c)
                out += L"\\\\";
            else if (L'\"' == c)
                out += L"\\\"";
            else if (L'/' == c)
                out += L"\\/";
            else if (L'\b' == c)
                out += L"\\b";
            else if (L'\r' == c)
                out += L"\\r";
            else if (L'\n' == c)
                out += L"\\n";
            else if (L'\t' == c)
                out += L"\\t";
            else if (c < L' ') {
                wchar_t buf[8];
                unsigned int u = c;
                std::swprintf (buf, sizeof(buf)/sizeof(buf[0]), L"\\u%04x", u);
                out += buf;
            }
            else
                out += c;
        }
        out += L"\"";
        break;
    case ARRAY:
        out += L"[";
        for (auto& x : marray) {
            if (count++ > 0)
                out += L", ";
            x.dump (out);
        }
        out += L"]";
        break;
    case OBJECT:
        out += L"{";
        for (auto& x : mobject) {
            if (count++ > 0)
                out += L", ";
            json key (x.first);
            key.dump (out);
            out += L": ";
            x.second.dump (out);
        }
        out += L"}";
        break;
    }
}

}//namespace wjson;
