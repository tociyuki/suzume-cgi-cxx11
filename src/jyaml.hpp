#ifndef DERVIS_H
#define DERVIS_H

#include <string>
#include <utility>
#include "wjson.hpp"

namespace wjson {

bool load_yaml (std::wstring const& input, json& value);

}//namespace wjson

#endif
