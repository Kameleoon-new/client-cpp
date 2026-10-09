#pragma once

#include "kameleoon/types/rule.hpp"
#include "kameleoon/types/variation.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace kameleoon
{
    struct FeatureFlag
    {
        bool environment_enabled = false;
        std::string default_variation_key;
        std::unordered_map<std::string, Variation> variations;
        std::vector<Rule> rules;

        [[nodiscard]] const Variation *default_variation() const;
    };
} // namespace kameleoon
