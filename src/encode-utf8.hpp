#pragma once

#include <ostream>
#include <string>
#include <cstdint>

namespace wjson {

bool encode_utf8 (std::wstring const& str, std::string& octets);
void encode_utf8 (std::ostream& out, std::uint32_t const uc);
bool decode_utf8 (std::string const& octets, std::wstring& str);
bool verify_utf8 (std::string const& octets);

}//namespace wjson
