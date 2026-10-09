#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kameleoon
{
    struct PageView
    {
        PageView(std::string url,
                 std::optional<std::string> title = std::nullopt,
                 std::vector<int32_t> referrers = {})
            : url(std::move(url)), title(std::move(title)), referrers(std::move(referrers)) {}

        std::string url;
        std::optional<std::string> title;
        std::vector<int32_t> referrers;
    };
} // namespace kameleoon
