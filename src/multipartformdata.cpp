#include <string>
#include <vector>
#include <cstdio>
#include <utility>
#include "http.hpp"
#include "encode-utf8.hpp"

namespace http {

struct media_type {
    media_type () : type (), parameter () {}
    bool match (std::string const& fieldvalue);
    std::size_t assoc (std::string const& attribute);
    std::string type;
    std::vector<std::string> parameter;
};

std::size_t
media_type::assoc (std::string const& attribute)
{
    for (std::size_t i = 0; i + 1 < parameter.size (); i += 2) {
        if (parameter[i] == attribute) {
            return i + 1;
        }
    }
    return 0;
}

static inline int
lowercase (int const c)
{
    return 'A' <= c && c <= 'Z' ? c + ('a' - 'A') : c;
}

bool
media_type::match (std::string const& fieldvalue)
{
    static const char CODE[] =
    //   @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_ !"#$%&'()*+,-./0123456789:;<=>?
        "@@@@@@@@@C@@@@@@@@@@@@@@@@@@@@@@CBGBBBBBIIBBIBBFBBBBBBBBBBIDIEII"
    //   @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
        "IBBBBBBBBBBBBBBBBBBBBBBBBBBIHIBBBBBBBBBBBBBBBBBBBBBBBBBBBBBIBIB@";
    static const char BASE[] = {
        0-2, 2-1, 6-2, 8-1, 14-1, 18-2, 12-2, 24-3, 20-2, 27-1,
        31-2, 39-2, 47-1,
    };
    static const unsigned short RULE[] = {
        0x132, 0x022, 0x013, 0x133, 0x063, 0x073, 0x154, 0x143,
        0x015, 0x155, 0x065, 0x075, 0x288, 0x098, 0x016, 0x5a8,
        0x066, 0x076, 0x287, 0x077, 0x3ba, 0x0aa,     0,     0,
        0x099, 0x0da, 0x5a9, 0x41b, 0x3bb, 0x46b, 0x47b, 0x3dc,
        0x3dc, 0x3dc, 0x3dc, 0x3dc, 0x3dc, 0x3dc, 0x3dc, 0x3dd,
        0x3dd, 0x3dd, 0x3dd, 0x3dd, 0x0ed, 0x0cd, 0x3dd, 0x41e,
            0, 0x46e, 0x47e,
    };
    static const int NRULE = sizeof (RULE) / sizeof (RULE[0]);
    type.clear ();
    parameter.clear ();
    bool isfilename = false;
    std::string attribute;
    std::string value;
    std::string::const_iterator s = fieldvalue.cbegin ();
    std::string::const_iterator const e = fieldvalue.cend ();
    int next_state = 2;
    for (; next_state >= 2 && s <= e; ++s) {
        unsigned int const octet = s == e ? '\0' : static_cast<unsigned char> (*s);
        int const code = s == e ? 1 : octet >= 128 ? 9
            : (0x0d == next_state && isfilename && '\\' == octet) ? 9
            : CODE[octet] - '@';
        int const i = BASE[next_state - 2] + code;
        int const rule = 0 <= i && i < NRULE ? RULE[i] : 0;
        next_state = (rule & 0x00fU) == next_state ? ((rule & 0x0f0U) >> 4) : 0;
        if (! next_state) {
            break;
        }
        switch (rule & 0xf00U) {
        case 0x100U:
            type.push_back (lowercase (octet));
            break;
        case 0x200U:
            attribute.push_back (lowercase (octet));
            break;
        case 0x300U:
            value.push_back (octet);
            break;
        case 0x400U:
            parameter.push_back (attribute);
            parameter.push_back (value);
            attribute.clear ();
            value.clear ();
            break;
        case 0x500U:
            isfilename = (attribute == "filename");
            break;
        }
    }
    return next_state == 1;
}

static std::string
disposition_name (std::string const& disposition)
{
    media_type media;
    if (media.match (disposition)) {
        if (media.assoc ("filename") > 0)
            return "";
        std::size_t i = media.assoc ("name");
        if (media.type == "form-data" && i > 0)
            return media.parameter[i];
    }
    return "";
}

static inline bool
matchtail (std::string const &s, std::string const &t)
{
    return s.size () >= t.size ()
            && s.compare (s.size () - t.size (), t.size (), t) == 0;
}

bool
formdata::ismultipart (std::string const& content_type)
{
    media_type media;
    if (! media.match (content_type)) {
        return false;
    }
    if (media.type != "multipart/form-data") {
        return false;
    }
    std::size_t const i = media.assoc ("boundary");
    if (! i) {
        return false;
    }
    std::swap (boundary, media.parameter[i]);
    return true;
}

bool
formdata::decode (FILE* in, std::size_t content_length)
{
    static const char CODE[] =
        "@@@@@@@@@CF@@D@@@@@@@@@@@@@@@@@@CAEAAAAAEEAAEAAEAAAAAAAAAABEEEEE"
        "EAAAAAAAAAAAAAAAAAAAAAAAAAAEEEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAEA@";
    static const char BASE[] = {0-1, 1-1, 3-1, 8-1, 13-6, 14-1, 15-6};
    static const unsigned short RULE[] = {
        0x121, 0x122, 0x032, 0x243, 0x243, 0x033, 0x053, 0x243,
        0x244, 0x244, 0x244, 0x054, 0x244, 0x065, 0x326, 0x087,
        0x246, 0x376,
    };
    static const int NRULE = sizeof (RULE) / sizeof (RULE[0]);
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
    std::wstring wname;
    std::wstring wbody;
    parameter.clear ();
    std::size_t count = 0;
    int next_state = 9;
    for (int ch; next_state < 11 && count < content_length && (ch = getc (in)) != EOF; ) {
        ++count;
        if (8 > next_state) {
            int const code = ch >= 128 ? 5 : CODE[ch] - '@';
            int const i = BASE[next_state - 1] + code;
            int const rule = 0 <= i && i < NRULE ? RULE[i] : 0;
            next_state = (rule & 0x00fU) == next_state ? ((rule & 0x0f0U) >> 4) : 0;
            if (! next_state)
                break;
            switch (rule & 0xf00U) {
            case 0x100U:
                fieldname.push_back (lowercase (ch));
                break;
            case 0x200U:
                fieldvalue.push_back (ch);
                break;
            case 0x300U:
                if (fieldname == "content-disposition" && name.empty ())
                    name = disposition_name (fieldvalue);
                fieldname.clear ();
                fieldvalue.clear ();
                if (1 == code)
                    fieldname.push_back (lowercase (ch));
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
                    if (name.empty ()
                            || ! wjson::decode_utf8 (name, wname)
                            || ! wjson::decode_utf8 (body, wbody)) {
                        next_state = 0;
                        break;
                    }
                    parameter.push_back (L"");
                    std::swap (parameter.back (), wname);
                    parameter.push_back (L"");
                    std::swap (parameter.back (), wbody);
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
    while (count < content_length && getc (in) != EOF) {
        ++count;
    }
    return 11 == next_state && count == content_length;
}

}//namespace http
