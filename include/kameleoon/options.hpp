#pragma once

#include "kameleoon/data/custom_data.hpp"

#include <cstdint>
#include <vector>

namespace kameleoon {

struct TrackConversionOptions {
    float revenue = 0.0F;
    bool negative = false;
    std::vector<CustomData> metadata;
};

struct IsFeatureActiveOptions {
    bool track = true;
};

struct GetVariationOptions {
    bool track = true;
};

struct GetVariationsOptions {
    bool only_active = false;
    bool track = true;
};

struct SetForcedVariationOptions {
    bool force_targeting = true;
};

}  // namespace kameleoon
