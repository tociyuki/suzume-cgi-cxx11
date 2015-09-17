#include <string>
#include <vector>
#include <istream>
#include <algorithm>
#include "encodeu8.hpp"

namespace http {

static inline std::size_t
assoc_media_param (std::vector<std::string> const& param, std::string const& name)
{
    for (std::size_t i = 0; i + 1 < param.size (); i += 2)
        if (param[i] == name)
            return i + 1;
    return 0;
}

static inline int
lowercase (int const c)
{
    return 'A' <= c && c <= 'Z' ? c + ('a' - 'A') : c;
}

static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const clsbpos = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[octet >> 3] >> clsbpos) & 0x0f) : 0;
}

static inline bool
matchtail (std::string const &s, std::string const &t)
{
    return s.size () >= t.size ()
            && s.compare (s.size () - t.size (), t.size (), t) == 0;
}

// media_type
//
//  tchar+ ('/' tchar+)? \s*
//  (';' \s* tchar+ \s*
//       '=' \s* (tchar+ | '"' (qchar | '\\' (qchar | ["\\]))* '"') \s*)*
//  \s*
//
//  tchar: [!#$%&'*+\-.^_`|~0-9A-Za-z]
//  qchar: [\x21\x23-\x5b\x5d-\x7e]  // exclude ["\\]
bool
decode_media_type (std::string const& fieldvalue,
    std::string& media_type, std::vector<std::string>& media_param)
{
    static const uint8_t SHIFT[15][10] = {
    //      t     /     v     \s    =     "     \\    ;     $
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S1
        {0, 0x12, 0x13, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x0e}, // S2
        {0, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S3
        {0, 0x14, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x0e}, // S4
        {0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x0e}, // S5
        {0, 0x27, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00}, // S6
        {0, 0x27, 0x00, 0x00, 0x08, 0x59, 0x00, 0x00, 0x00, 0x00}, // S7
        {0, 0x00, 0x00, 0x00, 0x08, 0x59, 0x00, 0x00, 0x00, 0x00}, // S8
        {0, 0x3a, 0x00, 0x00, 0x09, 0x00, 0x0c, 0x00, 0x00, 0x00}, // S9
        {0, 0x3a, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x46, 0x4e}, // Sa
        {0, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x00}, // Sb
        {0, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x0d, 0x0b, 0x3c, 0x00}, // Sc
        {0, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x46, 0x4e}, // Sd
        {1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Se
    };
    static uint32_t CCLASS[16] = {
    //                  tn  r
        0x00000000L, 0x04000000L, 0x00000000L, 0x00000000L,
    //     !"#$%&'     ()*+,-./     01234567     89:;<=>?
        0x41611111L, 0x33113112L, 0x11111111L, 0x11383533L,
    //    @ABCDEFG     HIJKLMNO     PQRSTUVW     XYZ[\]^_
        0x31111111L, 0x11111111L, 0x11111111L, 0x11137311L,
    //    `abcdefg     hijklmno     pqrstuvw     xyz{|}~
        0x11111111L, 0x11111111L, 0x11111111L, 0x11131310L,
    };
    bool isfilename = false;
    std::string attribute;
    std::string value;
    std::string::const_iterator s = fieldvalue.cbegin ();
    std::string::const_iterator const e = fieldvalue.cend ();
    int next_state = 1;
    for (; s <= e; ++s) {
        uint32_t const octet = s == e ? '\0' : static_cast<uint8_t> (*s);
        int const cls = s == e ? 9 : octet >= 128 ? 3
            : (0x0c == next_state && isfilename && '\\' == octet) ? 3
            : lookup_cls (CCLASS, octet);
        int const prev_state = next_state;
        next_state = 0 == cls ? 0 : (SHIFT[prev_state][cls] & 0x0f);
        if (! next_state)
            break;
        switch (SHIFT[prev_state][cls] & 0xf0) {
        case 0x10:
            media_type.push_back (lowercase (octet));
            break;
        case 0x20:
            attribute.push_back (lowercase (octet));
            break;
        case 0x30:
            value.push_back (octet);
            break;
        case 0x40:
            media_param.push_back (attribute);
            media_param.push_back (value);
            attribute.clear ();
            value.clear ();
            break;
        case 0x50:
            isfilename = attribute == "filename";
            break;
        }
    }
    return (SHIFT[next_state][0] & 1) != 0;
}

bool
is_multipart_formdata (std::string const& content_type, std::string& boundary)
{
    std::string media_type;
    std::vector<std::string> media_param;
    if (! decode_media_type (content_type, media_type, media_param))
        return false;
    if (media_type != "multipart/form-data")
        return false;
    std::size_t i = assoc_media_param (media_param, "boundary");
    if (! i)
        return false;
    std::swap (boundary, media_param[i]);
    return true;
}

static std::string
decode_disposition_name (std::string const& disposition)
{
    std::string media_type;
    std::vector<std::string> media_param;
    std::string name;
    if (decode_media_type (disposition, media_type, media_param)) {
        if (assoc_media_param (media_param, "filename") > 0)
            return "";
        std::size_t i = assoc_media_param (media_param, "name");
        if (media_type == "form-data" && i > 0)
            return media_param[i];
    }
    return "";
}

bool
decode_multipart_formdata (std::istream& input,
    std::size_t const content_length,
    std::string const& boundary,
    std::vector<std::wstring>& formdata)
{
    static const int SHIFT[9][7] = {
    //      t     v     \s    :     \r    \n
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0, 0x12, 0x00, 0X00, 0x00, 0x00, 0x00}, // S1: tchar S2
        {0, 0x12, 0x00, 0x00, 0x03, 0x00, 0x00}, // S2: tchar S2 | ':' S3
        {0, 0x24, 0x24, 0x03, 0x00, 0x05, 0x00}, // S3: vchar S4 | \s S3 | \r S5
        {0, 0x24, 0x24, 0x24, 0x24, 0x05, 0x00}, // S4: vchar S4 | \s S4 | \r S5
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06}, // S5: \n S6
        {0, 0x32, 0x00, 0x04, 0x00, 0x37, 0x00}, // S6: tchar S2 | \s S4 | \r S7
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08}, // S7: \n S8
        {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // S8: BODY
    };
    static uint32_t CCLASS[16] = { // [!#$%&'*+\-.^_`|~0-9A-Za-z]
    //                  tn  r
        0x00000000L, 0x03600500L, 0x00000000L, 0x00000000L,
    //     !"#$%&'     ()*+,-./     01234567     89:;<=>?
        0x31211111L, 0x22112112L, 0x11111111L, 0x11422222L,
    //    @ABCDEFG     HIJKLMNO     PQRSTUVW     XYZ[\]^_
        0x21111111L, 0x11111111L, 0x11111111L, 0x11122211L,
    //    `abcdefg     hijklmno     pqrstuvw     xyz{|}~
        0x11111111L, 0x11111111L, 0x11111111L, 0x11121210L,
    };
    static const char LF = '\x0a';
    static const std::string DASH = "--";
    static const std::string CRLF = "\x0d\x0a";
    std::string const dash_boundary = DASH + boundary + CRLF;
    std::string const delimiter = CRLF + DASH + boundary + CRLF;
    std::string const close_delimiter = CRLF + DASH + boundary + DASH + CRLF;
    std::string fieldname;
    std::string fieldvalue;
    std::string name;
    std::string body;
    std::size_t count = 0;
    int next_state = 9;
    for (char ch; next_state < 11 && count < content_length && input.get (ch); ) {
        ++count;
        if (8 > next_state) {
            uint32_t const octet = static_cast<uint8_t> (ch);
            int const cls = octet >= 128 ? 3 : lookup_cls (CCLASS, octet);
            int const prev_state = next_state;
            next_state = SHIFT[prev_state][cls] & 0x0f;
            if (! next_state)
                break;
            switch (SHIFT[prev_state][cls] & 0xf0) {
            case 0x10:
                fieldname.push_back (lowercase (octet));
                break;
            case 0x20:
                fieldvalue.push_back (octet);
                break;
            case 0x30:
                if (fieldname == "content-disposition" && name.empty ())
                    name = decode_disposition_name (fieldvalue);
                fieldname.clear ();
                fieldvalue.clear ();
                if (1 == cls)
                    fieldname.push_back (lowercase (octet));
                break;
            }
        }
        else if (8 == next_state) {
            body.push_back (ch);
            if (LF == ch) {
                if (matchtail (body, delimiter)) {
                    body.erase (body.size () - delimiter.size ());
                    next_state = 1;
                }
                else if (matchtail (body, close_delimiter)) {
                    body.erase (body.size () - close_delimiter.size ());
                    next_state = 11;
                }
                if (8 != next_state) {
                    if (name.empty ())
                        return false;
                    std::wstring wname;
                    std::wstring wbody;
                    if (! decode_utf8 (name, wname) || ! decode_utf8(body, wbody))
                        return false;
                    formdata.push_back (wname);
                    formdata.push_back (wbody);
                    name.clear ();
                    body.clear ();
                }
            }
        }
        else if (9 == next_state) {
            body.push_back (ch);
            if (LF == ch && body == dash_boundary) {
                next_state = 1;
                body.clear ();
            }
            else if (LF == ch) {
                next_state = 10;
            }
        }
        else if (10 == next_state) {
            body.push_back (ch);
            if (LF == ch && matchtail (body, delimiter)) {
                next_state = 1;
                body.clear ();
            }
        }
    }
    for (char ch; count < content_length && input.get (ch); ) {
        ++count;
    }
    return 11 == next_state && count == content_length;
}

}//namespace http
