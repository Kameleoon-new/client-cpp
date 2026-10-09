#pragma once

#include <optional>
#include <string>
#include <variant>

namespace kameleoon
{
    enum class JsonValueKind
    {
        Boolean,
        Number,
        String,
        JSON,
        JS,
        CSS,
    };

    /// Value of a feature variable. `String`, `JSON`, `JS` and `CSS` values
    /// are all carried as `std::string`; `kind` tells them apart.
    struct JsonValue
    {
        JsonValueKind kind;
        std::variant<bool, double, std::string> value;

        /// Typed accessors: `nullopt` when the value is of another kind.
        /// `as_string` covers every string-carrying kind (String, JSON, JS, CSS).
        [[nodiscard]] std::optional<bool> as_bool() const noexcept;
        [[nodiscard]] std::optional<double> as_number() const noexcept;
        [[nodiscard]] std::optional<std::string> as_string() const;

        static JsonValue boolean(bool value);
        static JsonValue number(double value);
        static JsonValue string(std::string value);
        static JsonValue json(std::string value);
        static JsonValue js(std::string value);
        static JsonValue css(std::string value);
    };
} // namespace kameleoon
