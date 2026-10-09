#pragma once

#include "kameleoon/data/custom_data.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace kameleoon
{
    struct Conversion
    {
        Conversion(uint32_t goal_id,
                   float revenue = 0.0F,
                   bool negative = false,
                   std::vector<CustomData> metadata = {})
            : goal_id(goal_id), revenue(revenue), negative(negative), metadata(std::move(metadata)) {}

        uint32_t goal_id;
        float revenue = 0.0F;
        bool negative = false;
        std::vector<CustomData> metadata;
    };
} // namespace kameleoon
