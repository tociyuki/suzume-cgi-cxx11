#include <string>
#include <locale>
#include "encodeu8.hpp"

std::wstring decode_utf8 (std::string octets)
{
    std::locale loc (std::locale::classic (), "ja_JP.UTF-8", std::locale::ctype);
    auto& cvt = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>> (loc);
    auto mb = std::mbstate_t ();
    std::wstring str (octets.size (), L'\0');
    char const* octetsnext;
    wchar_t* strnext;
    cvt.in (mb, &octets[0], &octets[octets.size ()], octetsnext,
                &str[0], &str[str.size ()], strnext);
    str.resize (strnext - &str[0]);
    return str;
}

std::string encode_utf8 (std::wstring str)
{
    std::locale loc (std::locale::classic (), "ja_JP.UTF-8", std::locale::ctype);
    auto& cvt = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>> (loc);
    auto mb = std::mbstate_t ();
    std::string octets(str.size () * cvt.max_length (), '\0');
    wchar_t const* strnext;
    char* octetsnext;
    cvt.out (mb, &str[0], &str[str.size ()], strnext,
                 &octets[0], &octets[octets.size ()], octetsnext);
    octets.resize (octetsnext - &octets[0]);
    return octets;
}

