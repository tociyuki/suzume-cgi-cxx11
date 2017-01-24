#include <string>
#include "http.hpp"

namespace http {

static int
digits_compare (std::string const& str1, std::size_t pos1, std::size_t len1,
                std::string const& str2, std::size_t pos2, std::size_t len2);

bool
content_length_type::canonlength (std::string const& str)
{
    static const char CODE[] =
        "@@@@@@@@@B@@@@@@@@@@@@@@@@@@@@@@B@@@@@@@@@@@C@@@DDDDDDDDDD@@@@@@";
    static const char BASE[] = {0-1, 4-2, 7-1, 11-1, 14-1};
    static const unsigned RULE[] = {
        0x12, 0x32, 0x32, 0x42, 0x33, 0x33, 0x43, 0x14, 0x54, 0x64, 0x44,
        0x15, 0x55, 0x65, 0x16, 0x66, 0x66, 0x46,
    };
    static const std::size_t NRULE = sizeof (RULE) / sizeof (RULE[0]);
    status = "400 Bad Request";
    std::size_t c1 = 0, c2 = 0, c3 = 0, c4 = 0;
    int next_state = 2;
    for (std::size_t sp = 0; next_state > 1 && sp <= str.size (); ++sp) {
        int const ch = sp == str.size () ? 0 : static_cast<unsigned char> (str[sp]);
        int const code = sp == str.size () ? 1 : ch < 64 ? CODE[ch] - '@' : 0;
        int const state = next_state;
        int const i = BASE[state - 2] + code;
        unsigned int rule = 0 <= i && i < NRULE ? RULE[i] : 0;
        next_state = (rule & 0x0fU) == state ? ((rule & 0xf0U) >> 4) : 0;
        if (2 == state && 1 == next_state) {
            status = "411 Length Required";
            return false;
        }
        else if (4 != state && 4 == next_state) {
            c3 = sp;
        }
        else if (4 == state && 4 != next_state) {
            c4 = sp;
            if (0 == c2)
                c1 = c3, c2 = c4;
            else if (digits_compare (str, c1, c2 - c1, str, c1, c2 - c1) != 0)
                next_state = 0;
        }
    }
    if (1 != next_state)
        return false;
    string.assign (str.cbegin () + c1, str.cbegin () + c2);
    status = "200 OK";
    return true;
}

bool
content_length_type::le (std::size_t limit) const
{
    if (status != "200 OK")
        return false;
    std::string const str2 = std::to_string (limit);
    return digits_compare (string, 0, string.size (), str2, 0, str2.size ()) <= 0;
}

std::size_t
content_length_type::to_size (void) const
{
    return std::stoul (string);
}

static int
digits_compare (std::string const& str1, std::size_t pos1, std::size_t len1,
                std::string const& str2, std::size_t pos2, std::size_t len2)
{
    while (len1 > 1 && '0' == str1[pos1]) {
        ++pos1;
        --len1;
    }
    while (len2 > 1 && '0' == str2[pos2]) {
        ++pos2;
        --len2;
    }
    if (len1 < len2)
        return -1;
    if (len1 > len2)
        return +1;
    return str1.compare (pos1, len1, str2, pos2, len2);   
}

}//namespace http
