#pragma once

#include "kameleoon/data/application_version.hpp"
#include "kameleoon/data/browser.hpp"
#include "kameleoon/data/conversion.hpp"
#include "kameleoon/data/cookie.hpp"
#include "kameleoon/data/custom_data.hpp"
#include "kameleoon/data/device.hpp"
#include "kameleoon/data/geolocation.hpp"
#include "kameleoon/data/operating_system.hpp"
#include "kameleoon/data/page_view.hpp"
#include "kameleoon/data/unique_identifier.hpp"
#include "kameleoon/data/user_agent.hpp"

#include <variant>

namespace kameleoon
{
    using Data = std::variant<Browser, Conversion, Cookie, CustomData, Device,
                              Geolocation, OperatingSystem, PageView,
                              UniqueIdentifier, UserAgent, ApplicationVersion>;

} // namespace kameleoon
