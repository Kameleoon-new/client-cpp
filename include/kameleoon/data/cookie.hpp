#pragma once

#include <string>
#include <unordered_map>

namespace kameleoon
{
    struct Cookie
    {
        std::unordered_map<std::string, std::string> cookies;
    };
} // namespace kameleoon
