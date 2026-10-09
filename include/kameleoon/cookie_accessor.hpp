#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kameleoon {

/// Bridges SDK cookie reads/writes to the host application (e.g. HTTP
/// request/response headers).
///
/// Contract: `get` must return a cookie already written through `set` on the
/// response in preference to the request cookie of the same name. The SDK keeps
/// no cookie state between calls, so this is what makes a second
/// `get_visitor_code` within the same request return the code written by the
/// first one instead of generating a new one.
///
/// Exception policy: implementations should not throw. `set` and `get` are
/// invoked from inside the SDK core; any exception escaping an override is
/// caught at the language boundary and discarded (a throwing `get` behaves as
/// if the cookie were absent).
/// Cookie values must be UTF-8; a malformed value returned by `get` is treated
/// as an absent cookie.
class CookieAccessor {
public:
    virtual ~CookieAccessor() = default;

    virtual void set(const std::string& key,
                     const std::string& value,
                     uint32_t max_age,
                     std::optional<std::string> top_level_domain) = 0;
    virtual std::optional<std::string> get(const std::string& key) = 0;
};

}  // namespace kameleoon
