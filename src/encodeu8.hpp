#ifndef ENCODEU8_H
#define ENCODEU8_H

#include <string>

std::wstring decode_utf8 (std::string octets);

std::string encode_utf8 (std::wstring str);

#endif
