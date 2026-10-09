#pragma once

#include "kameleoon/types/feature_flag.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace kameleoon
{
    struct DataFile
    {
        std::unordered_map<std::string, FeatureFlag> feature_flags;
        uint64_t date_modified = 0;
    };
} // namespace kameleoon
