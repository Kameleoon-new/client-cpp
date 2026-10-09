#pragma once

#include <optional>
#include <string>
#include <utility>

namespace kameleoon
{
    struct Geolocation
    {
        Geolocation(std::string country,
                    std::optional<std::string> region = std::nullopt,
                    std::optional<std::string> city = std::nullopt,
                    std::optional<std::string> postal_code = std::nullopt,
                    std::optional<float> latitude = std::nullopt,
                    std::optional<float> longitude = std::nullopt)
            : country(std::move(country)), region(std::move(region)), city(std::move(city)),
              postal_code(std::move(postal_code)), latitude(latitude), longitude(longitude) {}

        std::string country;
        std::optional<std::string> region;
        std::optional<std::string> city;
        std::optional<std::string> postal_code;
        std::optional<float> latitude;
        std::optional<float> longitude;
    };
} // namespace kameleoon
