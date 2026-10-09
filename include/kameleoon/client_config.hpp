#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kameleoon
{
    struct KameleoonClientConfig
    {
        std::string client_id;
        std::string client_secret;
        uint32_t refresh_interval_minutes = 60;
        uint32_t session_duration_minutes = 30;
        uint32_t default_timeout_millis = 10'000;
        uint32_t tracking_interval_millis = 1'000;
        std::optional<std::string> proxy_host;
        std::optional<std::string> environment;
        std::optional<std::string> top_level_domain;
        std::optional<std::string> network_domain;
    };
} // namespace kameleoon
