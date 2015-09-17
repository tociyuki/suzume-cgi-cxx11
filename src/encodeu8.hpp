#ifndef ENCODEU8_H
#define ENCODEU8_H

#include <string>

bool decode_utf8 (std::string const& octets, std::wstring& str);
bool encode_utf8 (std::wstring const& str, std::string& octets);

#endif
