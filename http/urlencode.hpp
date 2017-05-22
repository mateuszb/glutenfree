#pragma once

#include <string>

namespace http
{
std::string urldecode(const std::string& str);
std::string urlencode(const std::string& str);
}
