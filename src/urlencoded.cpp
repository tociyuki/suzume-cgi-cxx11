#include <string>
#include <vector>
#include <istream>
#include <sstream>
#include "encodeu8.hpp"

namespace http {
void push_pair (
    int const state,
    std::string& name, std::string& value,
    std::vector<std::wstring>& param)
{
    if (! name.empty ()) {
        if (state == 0) {
            param.push_back (L"name");
            param.push_back (decode_utf8 (name));
        }
        else {
            param.push_back (decode_utf8 (name));
            param.push_back (decode_utf8 (value));
        }
    }
    name.clear ();
    value.clear ();
}

bool decode_urlencoded (
    std::istream& input, std::size_t contentlength,
    std::vector<std::wstring>& param)
{
    int state = 0;
    int x = 0;
    char ch;
    std::size_t count = 0;
    std::string name;
    std::string value;
    while (count < contentlength && input.get (ch)) {
        ++count;
        if (state <= 1) {
            if ('+' == ch)
                ch = ' ';
            if ('%' == ch) {
                x = 0;
                state += 2;
            }
            else if (state == 0 && '=' == ch)
                state = 1;
            else if ('&' == ch) {
                push_pair (state, name, value, param);
                state = 0;
            }
            else if (state == 0)
                name.push_back (ch);
            else
                value.push_back (ch);
        }
        else {
            if ('0' <= ch && ch <= '9')
                x = (x << 4) + ch - '0';
            else if ('a' <= ch && ch <= 'f')
                x = (x << 4) + (ch - ('a' - 10));
            else if ('A' <= ch && ch <= 'F')
                x = (x << 4) + (ch - ('A' - 10));
            else
                return false;
            state += 2;
            if (state > 5) {
                state = state % 2;
                if (state == 0)
                    name.push_back (x);
                else
                    value.push_back (x);
            }
        }
    }
    if (state > 1)
        return false;
    push_pair (state, name, value, param);
    return true;
}

bool decode_urlencoded (
    std::string const& str, std::vector<std::wstring>& param)
{
    std::stringstream input (str);
    return decode_urlencoded(input, str.size (), param);
}
}//namespace http

