#include <string>
#include <vector>
#include <istream>
#include "encodeu8.hpp"

namespace http {
static inline bool istchar (int const c)
{
    // [!#$%&'*+\-.^_`|~0-9A-Za-z]
    static const unsigned long bitmap[8] = {
        0, 0x5f36ffc0UL, 0x7fffffe3UL, 0xffffffeaUL, 0, 0, 0, 0};
    if (c < 0 || c > 255)
        return false;
    int const sft = 31 - (c & 31);
    return ((bitmap[c >> 5] >> sft) & 1) != 0;
}

static inline int lowercase (int const c)
{
    return 'A' <= c && c <= 'Z' ? c + ('a' - 'A') : c;
}

static bool scan_header_word (
    std::string::const_iterator& s0, std::string::const_iterator const eos,
    std::string const& str)
{
    std::string::const_iterator s = s0;
    for (char c : str) {
        if (s >= eos || lowercase(*s) != c)
            return false;
        ++s;
    }
    while (s < eos && *s <= ' ')
        ++s;
    s0 = s;
    return true;
}

static bool scan_header_parameters (
    std::string::const_iterator s,
    std::string::const_iterator const eos,
    std::vector<std::string>& param)
{
    for (;;) {
        std::string name;
        std::string value;
        while (s < eos && *s <= ' ')
            ++s;
        if (! (s < eos && ';' == *s))
            break;
        ++s;
        while (s < eos && *s <= ' ')
            ++s;
        while (s < eos && istchar (static_cast<unsigned char> (*s)))
            name.push_back (lowercase (*s++));
        if (name.empty () || ! (s < eos && '=' == *s))
            return false;
        ++s;
        if (s < eos && '"' == *s) {
            ++s;
            for (;;) {
                if (! (s < eos && ('\t' == *s || ' ' <= *s)))
                    return false;
                int ch = *s++;
                if ('"' == ch)
                    break;
                if ('\\' == ch) {
                    if (! (s < eos && ' ' <= *s))
                        return false;
                    ch = *s++;
                }
                value.push_back (ch);
            }
        }
        else if (s < eos && istchar (static_cast<unsigned char> (*s))) {
            while (s < eos && istchar (static_cast<unsigned char> (*s)))
                value.push_back (*s++);
        }
        else
            return false;
        param.push_back (name);
        param.push_back (value);
    }
    return s == eos;
}

static inline bool matchtail (std::string const &s, std::string const &t)
{
    return s.size () >= t.size ()
            && s.compare (s.size () - t.size (), t.size (), t) == 0;
}

bool is_multipart_formdata (
    std::string const& content_type,
    std::string& boundary)
{
    std::vector<std::string> h;
    std::string::const_iterator s = content_type.cbegin ();
    std::string::const_iterator const eos = content_type.cend ();
    if (! scan_header_word (s, eos, "multipart/form-data"))
        return false;
    if (! scan_header_parameters (s, eos, h))
        return false;
    for (std::size_t i = 0; i < h.size (); i += 2)
        if (h[i] == "boundary") {
            boundary = h[i + 1];
            return true;
        }
    return false;
}

bool decode_multipart (
    std::istream& input, std::size_t const content_length,
    std::string const& boundary,
    std::vector<std::wstring>& param)
{
    static const int pattern_header[5][5] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 2},    /* S1: graph S2                     */
        {0, 3, 0, 2, 2},    /* S2: graph S2 | [ \t] S2 | CR S3  */
        {0, 0, 4, 0, 0},    /* S3: LF S4                        */
        {0,-1,-1, 2,-1}};   /* S4: [ \t] S2 |                   */
    std::string pattern0 = "--" + boundary + "\x0d\x0a";
    std::string pattern1 = "\x0d\x0a--" + boundary + "\x0d\x0a";
    std::string pattern2 = "\x0d\x0a--" + boundary + "--\x0d\x0a";
    int major_state = 0;
    int state = 0;
    std::size_t count = 0;
    std::string header;
    std::string name;
    std::string body;
    while (major_state < 3 && count < content_length) {
        if (0 == major_state) {         // leader
            int ch = input.get ();
            ++count;
            if (! (state < pattern0.size () && pattern0[state] == ch))
                return false;
            if (++state == pattern0.size ()) {
                major_state = 1;
                state = 1;
            }
        }
        else if (1 == major_state) {    // headers
            int ch = input.peek ();
            int ccls = (' ' < ch && '\x7f' != ch) ? 4
                     : '\x09' == ch ? 3 : ' ' == ch ? 3
                     : '\x0a' == ch ? 2 : '\x0d' == ch ? 1
                     : 0;
            state = pattern_header[state][ccls];
            if (1 <= state && state <= 4) {
                header.push_back (ch);
                input.get ();
                ++count;
            }
            else if (state < 0) {
                std::string::const_iterator s = header.cbegin ();
                std::string::const_iterator const eos = header.cend ();
                if (scan_header_word (s, eos, "content-disposition:")) {
                    std::vector<std::string> attr;
                    if (! scan_header_word (s, eos, "form-data"))
                        return false;
                    if (! scan_header_parameters (s, eos, attr))
                        return false;
                    for (std::size_t i = 0; i < attr.size (); i += 2)
                        if (attr[i] == "filename")  // diable file upload
                            return false;
                        else if (attr[i] == "name") {
                            name = attr[i + 1];
                            break;
                        }
                }
                header.clear ();
                state = 1;
            }
            else if (0 == state) {
                if (name.empty ())
                    return false;
                major_state = 2;
            }
        }
        else if (2 == major_state) {        // body
            int ch = input.get ();
            ++count;
            if (0 == state && '\x0d' != ch)
                return false;
            if (1 == state && '\x0a' != ch)
                return false;
            if (++state > 2)
                body.push_back (ch);
            if (matchtail (body, pattern1)) {
                body.erase (body.size () - pattern1.size ());
                state = 1;
                major_state = 1;
            }
            else if (matchtail (body, pattern2)) {
                body.erase (body.size () - pattern2.size ());
                major_state = 3;
            }
            if (2 != major_state) {
                param.push_back (decode_utf8 (name));
                param.push_back (decode_utf8 (body));
                body.clear ();
            }
        }
    }
    return count == content_length;
}
}//namespace http
