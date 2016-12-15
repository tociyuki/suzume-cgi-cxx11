#include <string>
#include <vector>
#include <utility>
#include "http.hpp"
#include "encode-utf8.hpp"

namespace http {

static inline uint32_t
xtoi (uint32_t const c)
{
    return '0' <= c && c <= '9' ? c - '0'
         : 'A' <= c && c <= 'F' ? c - 'A' + 10
         : 'a' <= c && c <= 'f' ? c - 'a' + 10
         : 0;
}

bool
formdata::decode_query_string (std::string const& query_string)
{
    static const char CODE[] =
    //   @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_ !"#$%&'()*+,-./0123456789:;<=>?
        "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@C@@CDFCCCCCCCCCBBBBBBBBBBCC@E@C"
    //   @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
        "CBBBBBBCCCCCCCCCCCCCCCCCCCC@@@@C@BBBBBBCCCCCCCCCCCCCCCCCCCC@@@C@";
    static const char BASE[] = {0-1, 4-2, 5-2, 6-1, 12-2, 13-2, 14-1, 20-2};
    static const unsigned short RULE[] = {
        0x012, 0x352, 0x352, 0x032, 0x143, 0x254, 0x415, 0x355,
        0x355, 0x035, 0x585, 0x495, 0x176, 0x287, 0x618, 0x388,
        0x388, 0x068, 0x388, 0x698, 0x359, 0x359, 0x039,
    };
    static const int NRULE = sizeof (RULE) / sizeof (RULE[0]);
    std::string name;
    std::string value;
    std::wstring wname;
    std::wstring wvalue;
    query_parameter.clear ();
    unsigned int xdigit = 0x30;  // '0'
    int next_state = 2;
    for (std::size_t pos = 0; next_state > 1 && pos <= query_string.size (); ++pos) {
        unsigned int const octet = pos == query_string.size () ? 0
            : static_cast<unsigned char> (query_string[pos]);
        int const code = pos == query_string.size () ? 1
            : octet < 128 ? CODE[octet] - '@': 0;
        int const i = BASE[next_state - 2] + code;
        int const rule = 0 <= i && i < NRULE ? RULE[i] : 0;
        next_state = (rule & 0x00fU) == next_state ? ((rule & 0x0f0U) >> 4) : 0;
        if (! next_state) {
            break;
        }
        switch (rule & 0xf00U) {
        case 0x100U:
            xdigit = octet;
            break;
        case 0x200U:
            value.push_back ((xtoi (xdigit) << 4) + xtoi (octet));
            break;
        case 0x300U:
            value.push_back ('+' == octet ? ' ' : octet);
            break;
        case 0x400U:
            if (! wjson::decode_utf8 (value, wvalue))
                return false;
            query_parameter.push_back (L".keyword");
            query_parameter.push_back (L"");
            std::swap (query_parameter.back (), wvalue);
            break;
        case 0x500U:
            std::swap (name, value);
            value.clear ();
            break;
        case 0x600U:
            if (! wjson::decode_utf8 (name, wname) || ! wjson::decode_utf8 (value, wvalue))
                return false;
            query_parameter.push_back (L"");
            std::swap (query_parameter.back (), wname);
            query_parameter.push_back (L"");
            std::swap (query_parameter.back (), wvalue);
            value.clear ();
            break;
        }
    }
    return 1 == next_state;
}

}//namespace http

