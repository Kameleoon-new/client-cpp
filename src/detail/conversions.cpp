#include "conversions.hpp"

#include "overloaded.hpp"
#include "result.hpp"

#include <limits>
#include <utility>
#include <variant>

using namespace std;

namespace kameleoon::detail
{
    // File-private helpers are `static` (not wrapped in anonymous namespaces)
    // so that the `to_ffi` overloads below share one scope with the public
    // ones declared in conversions.hpp and find each other by ordinary lookup.

    static float optional_float(optional<float> value)
    {
        return value.value_or(numeric_limits<float>::quiet_NaN());
    }

    ffi::FfiCustomData to_ffi(InputArena &arena, const CustomData &custom_data)
    {
        vector<ffi::BorrowedStr> values;
        values.reserve(custom_data.values.size());
        for (const auto &value : custom_data.values)
        {
            values.push_back(arena.raw(value));
        }

        return ffi::FfiCustomData{
            custom_data.id,
            arena.raw(custom_data.name),
            arena.array<ffi::FfiArray_BorrowedStr>(std::move(values)),
            custom_data.overwrite,
        };
    }

    static ffi::FfiConversion to_ffi(InputArena &arena, const Conversion &conversion)
    {
        vector<ffi::FfiCustomData> metadata;
        metadata.reserve(conversion.metadata.size());
        for (const auto &item : conversion.metadata)
        {
            metadata.push_back(to_ffi(arena, item));
        }

        return ffi::FfiConversion{
            conversion.goal_id,
            conversion.revenue,
            conversion.negative,
            arena.array<ffi::FfiArray_FfiCustomData>(std::move(metadata)),
        };
    }

    static ffi::FfiCookie to_ffi(InputArena &arena, const Cookie &cookie)
    {
        vector<ffi::FfiKeyValuePair_BorrowedStr__BorrowedStr> cookies;
        cookies.reserve(cookie.cookies.size());
        for (const auto &[key, value] : cookie.cookies)
        {
            cookies.push_back(ffi::FfiKeyValuePair_BorrowedStr__BorrowedStr{arena.raw(key), arena.raw(value)});
        }
        return ffi::FfiCookie{arena.array<ffi::FfiArray_FfiKeyValuePair_BorrowedStr__BorrowedStr>(std::move(cookies))};
    }

    static ffi::FfiGeolocation to_ffi(InputArena &arena, const Geolocation &geolocation)
    {
        return ffi::FfiGeolocation{
            arena.raw(geolocation.country),
            arena.raw(geolocation.region),
            arena.raw(geolocation.city),
            arena.raw(geolocation.postal_code),
            optional_float(geolocation.latitude),
            optional_float(geolocation.longitude),
        };
    }

    static ffi::FfiPageView to_ffi(InputArena &arena, const PageView &page_view)
    {
        return ffi::FfiPageView{
            arena.raw(page_view.url),
            arena.raw(page_view.title),
            arena.array<ffi::FfiArray_i32>(page_view.referrers),
        };
    }

    ffi::FfiData to_ffi(InputArena &arena, const Data &data)
    {
        ffi::FfiData result{};
        visit(
            Overloaded{
                [&](const Browser &value)
                {
                    if (static_cast<uint32_t>(value.type) > static_cast<uint32_t>(BrowserType::Other))
                        throw KameleoonException(ErrorCode::InvalidArgument, "Invalid Kameleoon BrowserType");
                    result.tag = ffi::Browser;
                    result.browser =
                        ffi::FfiBrowser{static_cast<ffi::FfiBrowserType>(value.type), optional_float(value.version)};
                },
                [&](const Conversion &value)
                {
                    result.tag = ffi::Conversion;
                    result.conversion = to_ffi(arena, value);
                },
                [&](const Cookie &value)
                {
                    result.tag = ffi::Cookie;
                    result.cookie = to_ffi(arena, value);
                },
                [&](const CustomData &value)
                {
                    result.tag = ffi::CustomData;
                    result.custom_data = to_ffi(arena, value);
                },
                [&](const Device &value)
                {
                    if (static_cast<uint32_t>(value.type) > static_cast<uint32_t>(DeviceType::Desktop))
                        throw KameleoonException(ErrorCode::InvalidArgument, "Invalid Kameleoon DeviceType");
                    result.tag = ffi::Device;
                    result.device = ffi::FfiDevice{static_cast<ffi::FfiDeviceType>(value.type)};
                },
                [&](const Geolocation &value)
                {
                    result.tag = ffi::Geolocation;
                    result.geolocation = to_ffi(arena, value);
                },
                [&](const OperatingSystem &value)
                {
                    if (static_cast<uint32_t>(value.type) > static_cast<uint32_t>(OperatingSystemType::WindowsPhone))
                        throw KameleoonException(ErrorCode::InvalidArgument, "Invalid Kameleoon OperatingSystemType");
                    result.tag = ffi::OperatingSystem;
                    result.operating_system =
                        ffi::FfiOperatingSystem{static_cast<ffi::FfiOperatingSystemType>(value.type)};
                },
                [&](const PageView &value)
                {
                    result.tag = ffi::PageView;
                    result.page_view = to_ffi(arena, value);
                },
                [&](const UniqueIdentifier &value)
                {
                    result.tag = ffi::UniqueIdentifier;
                    result.unique_identifier = ffi::FfiUniqueIdentifier{value.value};
                },
                [&](const UserAgent &value)
                {
                    result.tag = ffi::UserAgent;
                    result.user_agent = ffi::FfiUserAgent{arena.raw(value.value)};
                },
                [&](const ApplicationVersion &value)
                {
                    result.tag = ffi::ApplicationVersion;
                    result.application_version = ffi::FfiApplicationVersion{arena.raw(value.value)};
                },
            },
            data);
        return result;
    }

    ffi::FfiRemoteVisitorDataFilter to_ffi(const RemoteVisitorDataFilter &filter)
    {
        return ffi::FfiRemoteVisitorDataFilter{
            filter.previous_visit_amount,
            filter.current_visit,
            filter.custom_data,
            filter.page_views,
            filter.geolocation,
            filter.device,
            filter.browser,
            filter.operating_system,
            filter.conversions,
            filter.experiments,
            filter.kcs,
            filter.visitor_code,
            filter.personalizations,
            filter.cbs,
        };
    }

    ffi::FfiKameleoonClientConfig to_ffi(InputArena &arena, const KameleoonClientConfig &config)
    {
        return ffi::FfiKameleoonClientConfig{
            arena.raw(config.client_id),
            arena.raw(config.client_secret),
            config.refresh_interval_minutes,
            config.session_duration_minutes,
            config.default_timeout_millis,
            config.tracking_interval_millis,
            arena.raw(config.environment),
            arena.raw(config.proxy_host),
            arena.raw(config.top_level_domain),
            arena.raw(config.network_domain),
        };
    }

    static JsonValue copy_json_value(ffi::FfiJsonValue value)
    {
        switch (value.tag)
        {
        case ffi::Boolean:
            return JsonValue::boolean(value.boolean);
        case ffi::Number:
            return JsonValue::number(value.number);
        case ffi::String:
            return JsonValue::string(copy_raw(value.string));
        case ffi::JSON:
            return JsonValue::json(copy_raw(value.json));
        case ffi::JS:
            return JsonValue::js(copy_raw(value.js));
        case ffi::CSS:
            return JsonValue::css(copy_raw(value.css));
        }
        return JsonValue::string({});
    }

    static Variable copy_variable(ffi::FfiVariable value)
    {
        return Variable{
            copy_raw(value.key),
            copy_raw(value.kind),
            copy_json_value(value.value),
        };
    }

    Variation copy_variation(ffi::FfiVariation value)
    {
        vector<Variable> variables;
        variables.reserve(value.variables.len);
        for (uint32_t i = 0; i < value.variables.len; ++i)
        {
            variables.push_back(copy_variable(value.variables.ptr[i]));
        }

        return Variation{
            copy_raw(value.key),
            copy_raw(value.name),
            value.id == NO_U32 ? optional<uint32_t>{} : value.id,
            value.experiment_id == NO_U32 ? optional<uint32_t>{} : value.experiment_id,
            std::move(variables),
        };
    }

    unordered_map<string, Variation> copy_variation_map(ffi::FfiVariationMap values)
    {
        unordered_map<string, Variation> result;
        result.reserve(values.len);
        for (uint32_t i = 0; i < values.len; ++i)
        {
            const auto &pair = values.ptr[i];
            result.try_emplace(copy_raw(pair.key), copy_variation(pair.value));
        }
        return result;
    }

    static Rule copy_rule(ffi::FfiRule value)
    {
        return Rule{copy_variation_map(value.variations)};
    }

    static FeatureFlag copy_feature_flag(ffi::FfiFeatureFlag value)
    {
        vector<Rule> rules;
        rules.reserve(value.rules.len);
        for (uint32_t i = 0; i < value.rules.len; ++i)
        {
            rules.push_back(copy_rule(value.rules.ptr[i]));
        }

        return FeatureFlag{
            value.environment_enabled,
            copy_raw(value.default_variation_key),
            copy_variation_map(value.variations),
            std::move(rules),
        };
    }

    DataFile copy_datafile(ffi::FfiDataFile value)
    {
        DataFile datafile;
        datafile.date_modified = value.date_modified;
        datafile.feature_flags.reserve(value.feature_flags.len);

        for (uint32_t i = 0; i < value.feature_flags.len; ++i)
        {
            const auto &pair = value.feature_flags.ptr[i];
            datafile.feature_flags.try_emplace(copy_raw(pair.key), copy_feature_flag(pair.value));
        }

        return datafile;
    }

} // namespace kameleoon::detail
