#pragma once

#include <ostream>
#include <string>

namespace wjson {

bool decode_utf8 (std::string const& octets, std::wstring& str);
bool encode_utf8 (std::wstring const& str, std::string& octets);

static inline void
encode_utf8 (std::ostream& out, uint32_t const uc)
{
    if (uc < 0x80)
        out.put (uc);
    else if (uc < 0x800) {
        out.put (((uc >>  6) & 0xff) | 0xc0);
        out.put (( uc        & 0x3f) | 0x80);
    }
    else if (uc < 0x10000) {
        out.put (((uc >> 12) & 0x0f) | 0xe0);
        out.put (((uc >>  6) & 0x3f) | 0x80);
        out.put (( uc        & 0x3f) | 0x80);
    }
    else if (uc < 0x110000) {
        out.put (((uc >> 18) & 0x07) | 0xf0);
        out.put (((uc >> 12) & 0x3f) | 0x80);
        out.put (((uc >>  6) & 0x3f) | 0x80);
        out.put (( uc        & 0x3f) | 0x80);
    }
}

}//namespace wjson
