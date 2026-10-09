#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace kameleoon {

enum class LogLevel : uint8_t {
    None = 0,
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
};

class KameleoonLogger {
public:
    /// Callable invoked for every log record emitted by the SDK core.
    ///
    /// The logger may be invoked from internal SDK threads. Resetting or
    /// replacing it does not wait for invocations already in progress. It must be
    /// thread-safe and must not throw; any exception escaping the logger is
    /// caught and discarded by the SDK.
    /// Do not block on SDK operations from a logger, or call methods that emit
    /// logs recursively.
    using Logger = std::function<void(LogLevel level, const std::string& message)>;

    static void set_log_level(LogLevel level);

    /// Registers a global logger. Passing an empty logger (`{}` or `nullptr`)
    /// removes the registered one and restores the default (stdout) logger of
    /// the SDK core.
    static void set_logger(Logger logger);
};

}  // namespace kameleoon
