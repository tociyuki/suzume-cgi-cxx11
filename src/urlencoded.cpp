#include <string>
#include <vector>
#include <istream>
#include <sstream>
#include <utility>
#include "encodeu8.hpp"

namespace http {
static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const i = octet >> 3;
    uint32_t const count = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[i] >> count) & 0x0f) : 0;
}

static inline uint32_t
xtoi (uint32_t const c)
{
    return '0' <= c && c <= '9' ? c - '0'
         : 'A' <= c && c <= 'F' ? c - 'A' + 10
         : 'a' <= c && c <= 'f' ? c - 'a' + 10
         : 0;
}

bool
decode_urlencoded (
    std::istream& input, std::size_t const length,
    std::vector<std::wstring>& param)
{
    static const int SHIFT[10][7] = {
    //      any   hex   '%'   '='   '&'   $
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x34, 0x34, 0x02, 0x00, 0x00, 0x09}, // S1
        {0, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00}, // S2
        {0, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00}, // S3
        {0, 0x34, 0x34, 0x02, 0x57, 0x48, 0x49}, // S4
        {0, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00}, // S5
        {0, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00}, // S6
        {0, 0x37, 0x37, 0x05, 0x37, 0x68, 0x69}, // S7
        {0, 0x34, 0x34, 0x02, 0x00, 0x00, 0x00}, // S8
        {1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S9
    };
    static const uint32_t CCLASS[16] = {
    //                 tn  r                          
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    //     !"#$%&'    ()*+,-./    01234567    89:;<=>?
        0x01001351, 0x11111111, 0x22222222, 0x22110401,
    //    @ABCDEFG    HIJKLMNO    PQRSTUVW    XYZ[\]^_
        0x12222221, 0x11111111, 0x11111111, 0x11100001,
    //    `abcdefg    hijklmno    pqrstuvw    xyz{|}~ 
        0x02222221, 0x11111111, 0x11111111, 0x11100010,
    };
    enum { DOLLAR = 6 };
    std::string name;
    std::string value;
    std::wstring wname;
    std::wstring wvalue;
    uint32_t xdigit = '0';
    int next_state = 1;
    for (std::size_t count = 0; count <= length; ++count) {
        char ch;
        if (count < length && ! input.get (ch))
            break;
        uint32_t const octet = count == length ? '\0' : static_cast<uint8_t> (ch);
        int const cls = count == length ? DOLLAR : lookup_cls (CCLASS, octet);
        int const prev_state = next_state;
        next_state = 0 == cls ? 0 : (SHIFT[prev_state][cls] & 0x0f);
        if (! next_state)
            break;
        switch (SHIFT[prev_state][cls] & 0xf0) {
        case 0x10:
            xdigit = octet;
            break;
        case 0x20:
            value.push_back ((xtoi (xdigit) << 4) + xtoi (octet));
            break;
        case 0x30:
            value.push_back ('+' == octet ? ' ' : octet);
            break;
        case 0x40:
            if (! decode_utf8 (value, wvalue))
                return false;
            param.push_back (L"name");
            param.push_back (L""); std::swap (param.back (), wvalue);
            value.clear ();
            break;
        case 0x50:
            std::swap (name, value);
            value.clear ();
            break;
        case 0x60:
            if (! decode_utf8 (name, wname) || ! decode_utf8 (value, wvalue))
                return false;
            param.push_back (L""); std::swap (param.back (), wname);
            param.push_back (L""); std::swap (param.back (), wvalue);
            value.clear ();
            break;
        }
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

