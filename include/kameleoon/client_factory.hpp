#pragma once

#include "kameleoon/client.hpp"
#include "kameleoon/client_config.hpp"
#include "kameleoon/constants.hpp"

#include <string>

namespace kameleoon
{
    /// Creates and caches `KameleoonClient` instances.
    ///
    /// Clients are cached per (site code, environment): repeated `create`
    /// calls with the same key return wrappers over the same underlying
    /// client, and the configuration of the first call wins. Use `forget` to
    /// drop a cached client and release its background resources.
    class KameleoonClientFactory
    {
    public:
        /// Creates (or returns the cached) client for `site_code` using the
        /// given configuration. The returned client is not ready to serve
        /// evaluations until `initialize` has completed.
        static KameleoonClient create(const std::string &site_code, const KameleoonClientConfig &config);
        /// Creates (or returns the cached) client for `site_code`, reading the
        /// configuration from a JSON file at `config_path`. Throws
        /// `KameleoonException` (`ErrorCode::Initialization`) if the file
        /// cannot be read or parsed.
        static KameleoonClient create(const std::string &site_code, const std::string &config_path);
        /// Removes the cached client for `site_code` (default environment).
        static void forget(const std::string &site_code);
        /// Removes the cached client for `site_code` in the given environment.
        static void forget(const std::string &site_code, const std::string &environment);

    private:
        static KameleoonClient wrap(const void *inner);
    };
} // namespace kameleoon
