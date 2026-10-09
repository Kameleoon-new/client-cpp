#pragma once

#include <cstdint>
#include <optional>

namespace kameleoon
{
    enum class BrowserType : uint32_t
    {
        Chrome = 0,
        InternetExplorer = 1,
        Firefox = 2,
        Safari = 3,
        Opera = 4,
        Other = 5,
    };

    struct Browser
    {
        // Constructors (rather than aggregate initialization) let callers omit
        // the optional members without -Wmissing-field-initializers.
        Browser(BrowserType type, std::optional<float> version = std::nullopt)
            : type(type), version(version) {}

        BrowserType type;
        std::optional<float> version;
    };
} // namespace kameleoon
