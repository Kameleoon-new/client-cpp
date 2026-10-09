#include "kameleoon/types/json_value.hpp"

#include <utility>

namespace kameleoon
{

    std::optional<bool> JsonValue::as_bool() const noexcept
    {
        const auto *value_ptr = std::get_if<bool>(&value);
        return value_ptr == nullptr ? std::nullopt : std::optional<bool>(*value_ptr);
    }

    std::optional<double> JsonValue::as_number() const noexcept
    {
        const auto *value_ptr = std::get_if<double>(&value);
        return value_ptr == nullptr ? std::nullopt : std::optional<double>(*value_ptr);
    }

    std::optional<std::string> JsonValue::as_string() const
    {
        const auto *value_ptr = std::get_if<std::string>(&value);
        return value_ptr == nullptr ? std::nullopt : std::optional<std::string>(*value_ptr);
    }

    JsonValue JsonValue::boolean(bool value)
    {
        return JsonValue{JsonValueKind::Boolean, value};
    }

    JsonValue JsonValue::number(double value)
    {
        return JsonValue{JsonValueKind::Number, value};
    }

    JsonValue JsonValue::string(std::string value)
    {
        return JsonValue{JsonValueKind::String, std::move(value)};
    }

    JsonValue JsonValue::json(std::string value)
    {
        return JsonValue{JsonValueKind::JSON, std::move(value)};
    }

    JsonValue JsonValue::js(std::string value)
    {
        return JsonValue{JsonValueKind::JS, std::move(value)};
    }

    JsonValue JsonValue::css(std::string value)
    {
        return JsonValue{JsonValueKind::CSS, std::move(value)};
    }

} // namespace kameleoon
