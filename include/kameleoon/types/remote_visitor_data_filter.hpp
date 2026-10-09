#pragma once

#include <cstdint>

namespace kameleoon
{
    struct RemoteVisitorDataFilter
    {
        uint32_t previous_visit_amount = 1;
        bool current_visit = true;
        bool custom_data = true;
        bool page_views = false;
        bool geolocation = false;
        bool device = false;
        bool browser = false;
        bool operating_system = false;
        bool conversions = false;
        bool experiments = false;
        bool kcs = false;
        bool visitor_code = true;
        bool personalizations = false;
        bool cbs = false;
    };
} // namespace kameleoon
