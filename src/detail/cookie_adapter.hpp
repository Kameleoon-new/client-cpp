#pragma once

#include "kameleoon/cookie_accessor.hpp"
#include "ffi.hpp"

namespace kameleoon::detail
{

    ffi::FfiCookieAccessor cookie_accessor(CookieAccessor &accessor);

} // namespace kameleoon::detail
