#ifndef DERVIS_H
#define DERVIS_H

#include <string>
#include <utility>
#include "wjson.hpp"

namespace wjson {

std::wstring::size_type load_yaml (std::wstring const& input, json& value);

}//namespace wjson

#endif
