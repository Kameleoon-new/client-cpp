#pragma once

#include "kameleoon/types/json_value.hpp"

#include <string>

namespace kameleoon
{
    struct Variable
    {
        std::string key;
        std::string kind;
        JsonValue value;
    };
} // namespace kameleoon
