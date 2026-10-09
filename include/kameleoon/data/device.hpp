#pragma once

#include <cstdint>

namespace kameleoon
{
    enum class DeviceType : uint32_t
    {
        Phone = 0,
        Tablet = 1,
        Desktop = 2,
    };

    struct Device
    {
        DeviceType type;
    };
} // namespace kameleoon
