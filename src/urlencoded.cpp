#include <string>
#include <vector>
#include <istream>
#include <sstream>
#include "encodeu8.hpp"

namespace http {
static inline int
xtoi (int const ch)
{
    if ('a' <= ch)
        return ch - 'a' + 10;
    if ('A' <= ch)
        return ch - 'A' + 10;
    return ch - '0';
}

bool
decode_urlencoded (std::istream& input, std::size_t length,
    std::vector<std::wstring>& param)
{
    static const int8_t SHIFT[16][8] = {
    //          &   =   %   +   x   p   $
        {   0,  0,  0,  0,  0,  0,  0,  0},
        {   0,  0,  0,  2,  5,  5,  5, 15}, // S1: '%' S2 | '+' S5 | p S5 | $ S15
        {   0,  0,  0,  0,  0,  3,  0,  0}, // S2: x S3
        {   0,  0,  0,  0,  0,  4,  0,  0}, // S3: x S4
        {   0,  7,  8,  2,  5,  5,  5,  6}, // S4: '&' S7 | '=' S8 | '%' S2 | '+' S5 | p S5 | $ S6
        {   0,  7,  8,  2,  5,  5,  5,  6}, // S5: '&' S7 | '=' S8 | '%' S2 | '+' S5 | p S5 | $ S6
        {   1,  0,  0,  0,  0,  0,  0,  0}, // S6: MATCH
        {   0,  0,  0,  2,  5,  5,  5,  0}, // S7: '%' S2 | '+' S5 | p S5
        {   0, 14, 12,  9, 12, 12, 12, 13}, // S8: '&' S14 | '=' S12 | '%' S9 | '+' S12 | p S12 | $ S13
        {   0,  0,  0,  0,  0, 10,  0,  0}, // S9: x S10
        {   0,  0,  0,  0,  0, 11,  0,  0}, // S10: x S11
        {   0, 14, 12,  9, 12, 12, 12, 13}, // S11: '&' S14 | '=' S12 | '%' S9 | '+' S12 | p S12 | $ S13
        {   0, 14, 12,  9, 12, 12, 12, 13}, // S12: '&' S14 | '=' S12 | '%' S9 | '+' S12 | p S12 | $ S13
        {   1,  0,  0,  0,  0,  0,  0,  0}, // S13: MATCH
        {   0,  0,  0,  2,  5,  5,  5, 15}, // S14: '%' S2 | '+' S5 | p S5 | $ S15
        {   1,  0,  0,  0,  0,  0,  0,  0}, // S15: MATCH
    };
    static const int8_t CCLASS[128] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    //      !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
        0,  6,  0,  0,  6,  3,  1,  6,  6,  6,  6,  4,  6,  6,  6,  6,
    //  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
        5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  0,  2,  0,  6,
    //  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
        6,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    //  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
        6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  0,  0,  0,  6,
    //  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
        0,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,  6,
    //  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~
        6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  0,  0,  6,  0,
    };
    enum { DOLLAR = 7 };
    std::string name;
    std::string value;
    std::wstring wname;
    std::wstring wvalue;
    int next_state = 1;
    int xdigit = '0';
    for (;;) {
        char ch;
        if (length > 0 && ! input.get (ch))
            return false;
        int octet = 0 == length ? '\0' : static_cast<uint8_t> (ch);
        int const cls = 0 == length ? DOLLAR : octet < 128 ? CCLASS[octet] : 0;
        if (length > 0)
            --length;
        next_state = 0 == cls ? 0 : SHIFT[next_state][cls];
        if ('+' == octet)
            octet = ' ';
        switch (next_state) {
        case 0:
            return false;
        case 3:
        case 10:
            xdigit = octet;
            break;
        case 4:
        case 11:
            value.push_back ((xtoi (xdigit) << 4) + xtoi (octet));
            break;
        case 5:
        case 12:
            value.push_back (octet);
            break;
        case 6:
        case 7:
            if (! decode_utf8 (value, wvalue))
                return false;
            param.push_back (L"name");
            param.push_back (wvalue);
            value.clear ();
            break;
        case 8:
            std::swap (name, value);
            value.clear ();
            break;
        case 13:
        case 14:
            if (! decode_utf8 (name, wname) || ! decode_utf8 (value, wvalue))
                return false;
            param.push_back (wname);
            param.push_back (wvalue);
            name.clear ();
            value.clear ();
            break;
        }
        if (DOLLAR == cls)
            break;
    }
    return (SHIFT[next_state][0] & 1) != 0;
}

bool decode_urlencoded (
    std::string const& str, std::vector<std::wstring>& param)
{
    std::stringstream input (str);
    return decode_urlencoded(input, str.size (), param);
}
}//namespace http

