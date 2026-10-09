#pragma once

#include <cstdint>

namespace kameleoon
{
    enum class OperatingSystemType : uint32_t
    {
        Windows = 0,
        Mac = 1,
        IOS = 2,
        Linux = 3,
        Android = 4,
        WindowsPhone = 5,
    };

    struct OperatingSystem
    {
        OperatingSystemType type;
    };
} // namespace kameleoon
