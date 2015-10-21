#include <string>
#include <utility>
#include "encode-utf8.hpp"

namespace wjson {

bool
encode_utf8 (std::wstring const& str, std::string& octets)
{
    std::string buf;
    octets.clear ();
    for (auto s = str.cbegin (); s != str.cend (); ++s) {
        uint32_t code = static_cast<uint32_t> (*s);
        if (code < 0x80) {
            buf.push_back (code);
        }
        else if (code < 0x800) {
            buf.push_back (((code >>  6) & 0xff) | 0xc0);
            buf.push_back (( code        & 0x3f) | 0x80);
        }
        else if (code < 0x10000) {
            buf.push_back (((code >> 12) & 0x0f) | 0xe0);
            buf.push_back (((code >>  6) & 0x3f) | 0x80);
            buf.push_back (( code        & 0x3f) | 0x80);
        }
        else if (code < 0x110000) {
            buf.push_back (((code >> 18) & 0x07) | 0xf0);
            buf.push_back (((code >> 12) & 0x3f) | 0x80);
            buf.push_back (((code >>  6) & 0x3f) | 0x80);
            buf.push_back (( code        & 0x3f) | 0x80);
        }
        else
            return false;
    }
    std::swap (octets, buf);
    return true;
}

bool
decode_utf8 (std::string const& octets, std::wstring& str)
{
    static const int SHIFT[5][6] = {
    //     00  c0  e0  f0  80 
        {0, 0,  0,  0,  0,  0},
        {0, 1,  2,  3,  4,  0}, // S1: 00 S1 | c0 S2 | e0 S3 | f0 S4
        {0, 0,  0,  0,  0,  1}, // S2: 80 S1
        {0, 0,  0,  0,  0,  2}, // S3: 80 S2
        {0, 0,  0,  0,  0,  3}, // S4: 80 S3
    };
    static const uint32_t LOWERBOUND[5] = {0, 0, 0x80L, 0x0800L, 0x10000L};
    static const uint32_t UPPERBOUND = 0x10ffffL;
    std::wstring buf;
    str.clear ();
    int state = 1;
    int nbyte = 1;
    uint32_t code = 0;
    for (auto s = octets.cbegin (); s != octets.cend (); ++s) {
        uint32_t const octet = static_cast<uint8_t> (*s);
        int const cls = 0 == (0x80 & octet) ? 1
                   : 0xc0 == (0xe0 & octet) ? 2
                   : 0xe0 == (0xf0 & octet) ? 3
                   : 0xf0 == (0xf8 & octet) ? 4
                   : 0x80 == (0xc0 & octet) ? 5
                   : 0;
        state = 0 == cls ? 0 : SHIFT[state][cls];
        if (! state)
            return false;
        if (cls < 5)
            nbyte = cls;
        switch (cls) {
        case 1: code = octet; break;
        case 2: code = 0x1f & octet; break;
        case 3: code = 0x0f & octet; break;
        case 4: code = 0x07 & octet; break;
        case 5: code = (code << 6) | (0x3f & octet); break;
        }
        if (1 == state) {
            if (code < LOWERBOUND[nbyte] || UPPERBOUND < code)
                return false;
            if (0xd800L <= code && code <= 0xdfffL) // UTF-16 Surrogate Code
                return false;
            buf.push_back (code);
        }
    }
    if (1 == state)
        std::swap (str, buf);
    return 1 == state;
}

}//namespace wjson
