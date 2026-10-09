#pragma once

#include "kameleoon/types/variation.hpp"

#include <string>
#include <unordered_map>

namespace kameleoon
{
    struct Rule
    {
        std::unordered_map<std::string, Variation> variations;
    };
} // namespace kameleoon
