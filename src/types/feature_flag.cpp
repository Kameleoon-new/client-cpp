#include "kameleoon/types/feature_flag.hpp"

namespace kameleoon
{

    const Variation *FeatureFlag::default_variation() const
    {
        auto it = variations.find(default_variation_key);
        return it == variations.end() ? nullptr : &it->second;
    }

} // namespace kameleoon
