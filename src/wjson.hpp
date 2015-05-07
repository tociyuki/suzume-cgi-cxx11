#ifndef WJSON_H
#define WJSON_H

#include <vector>
#include <map>
#include <string>

namespace wjson {

class json final {
public:
    enum variation {JSONNULL, BOOLEAN, INTEGER, NUMBER, STRING, ARRAY, OBJECT};
    typedef std::vector<json> array;
    typedef std::map<std::wstring, json> object;
    json ();
    json (json const& x);
    json (json&& x);
    ~json ();
    json (std::nullptr_t x);
    json (bool x);
    json (int x);
    json (double x);
    json (wchar_t const* x);
    json (std::wstring const& x);
    json (std::wstring&& x);
    json (array const& x);
    json (array&& x);
    json (object const& x);
    json (object&& x);
    json& operator=(json const& x);
    json& operator=(json&& x);
    void replace (json const& x);
    void swap (json& x);
    template<typename T> bool is () const;
    template<typename T> T const& as () const;
    template<typename T> T& as ();
    json const& operator[] (size_t idx) const;
    json& operator[] (size_t idx);
    json const& operator[] (std::wstring const& key) const;
    json& operator[] (std::wstring const& key);
    void dump (std::wstring& out) const;
    std::wstring dump () const;
    variation guard () const;

private:
    variation mguard;
    union {
        bool mboolean;
        int minteger;
        double mnumber;
        std::wstring mstring;
        array marray;
        object mobject;
    };

    void copy_data (json const& x);
    void move_data (json&& x);
    void destroy ();
};

typedef json::array array;
typedef json::object object;

class loader final {
public:
    typedef std::wstring::const_iterator cursor;
    loader ();
    ~loader ();
    bool parse (std::wstring const& src, json& node);

private:
    loader (loader const& x);
    loader& operator=(loader const& x);
    int next_token (cursor& s, cursor const& eos, json& node);
    bool scan_number (cursor& s, cursor const& eos, json& node);
    bool scan_string (cursor& s, cursor const& eos, json& node);
    bool decode_u16 (cursor& s, cursor const& eos, wchar_t& c);
    bool isjsonspace (wchar_t const c);
    bool isjsondigit (wchar_t const c);
};

inline bool load (std::wstring const& input, json& node)
{
    loader ctx;
    return ctx.parse (input, node);
}

}//namespace wjson;
#endif
