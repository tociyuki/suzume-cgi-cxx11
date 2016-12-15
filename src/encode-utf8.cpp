#include <string>
#include <utility>
#include <cstdint>
#include "encode-utf8.hpp"

namespace wjson {

bool
encode_utf8 (std::wstring const& str, std::string& octets)
{
    std::string buf;
    octets.clear ();
    for (auto s = str.cbegin (); s != str.cend (); ++s) {
        std::uint32_t code = static_cast<std::uint32_t> (*s);
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
    static const std::uint32_t LOWERBOUND[5] = {0, 0, 0x80L, 0x0800L, 0x10000L};
    static const std::uint32_t UPPERBOUND = 0x10ffffL;
    std::wstring buf;
    str.clear ();
    int state = 1;
    int length = 1;
    std::uint32_t code = 0;
    for (auto s = octets.cbegin (); state > 0 && s != octets.cend (); ++s) {
        std::uint32_t octet = static_cast<unsigned char> (*s);
        if (0 == (0x80U & octet)) {
            length = state = (1 == state) ? 1 : 0;
            code = octet;
        }
        else if (0xc0U == (0xe0U & octet)) {
            length = state = (1 == state) ? 2 : 0;
            code = 0x1f & octet;
        }
        else if (0xe0U == (0xf0U & octet)) {
            length = state = (1 == state) ? 3 : 0;
            code = 0x0f & octet;
        }
        else if (0xf0U == (0xf8U & octet)) {
            length = state = (1 == state) ? 4 : 0;
            code = 0x07 & octet;
        }
        else if (0x80U == (0xc0U & octet)) {
            state = (1 < state) ? state - 1 : 0;
            code = (code << 6) | (0x3f & octet);
        }
        if (1 == state) {
            if (code < LOWERBOUND[length] || UPPERBOUND < code)
                return false;
            if (0xd800L <= code && code <= 0xdfffL)
                return false;
            buf.push_back (code);
        }
    }
    if (1 == state)
        std::swap (str, buf);
    return 1 == state;
}

}//namespace wjson
