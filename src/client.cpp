#include "kameleoon/client.hpp"

#include "detail/client_callback_state.hpp"
#include "detail/completion.hpp"
#include "detail/conversions.hpp"
#include "detail/cookie_adapter.hpp"
#include "detail/event_handlers.hpp"
#include "detail/input_arena.hpp"
#include "detail/overloaded.hpp"
#include "detail/result.hpp"

#include "detail/ffi.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace std;

namespace kameleoon
{
    namespace ffi = detail::ffi;

    namespace
    {
        // Validates a user timeout and converts it to the FFI representation,
        // where 0 selects the configured default. Throws InvalidArgument
        // on a non-positive timeout.
        uint64_t checked_timeout_millis(const optional<chrono::milliseconds> &timeout)
        {
            if (!timeout.has_value())
            {
                return 0; // Core falls back to the configured default timeout.
            }
            if (timeout->count() <= 0)
            {
                throw KameleoonException(ErrorCode::InvalidArgument, "Kameleoon timeout must be positive");
            }
            return static_cast<uint64_t>(timeout->count());
        }

    } // namespace

    KameleoonClient::KameleoonClient(const void *inner)
        : inner_(inner), callback_state_(make_shared<detail::ClientCallbackState>())
    {
        // Transfer ownership only after allocation succeeds; the factory
        // releases the native reference if construction throws.
        callback_state_->inner = inner;
    }

    KameleoonClient::~KameleoonClient()
    {
        release();
    }

    KameleoonClient::KameleoonClient(KameleoonClient &&other) noexcept
        : inner_(other.inner_), callback_state_(std::move(other.callback_state_))
    {
        other.inner_ = nullptr;
    }

    KameleoonClient &KameleoonClient::operator=(KameleoonClient &&other) noexcept
    {
        if (this != &other)
        {
            release();
            inner_ = other.inner_;
            callback_state_ = std::move(other.callback_state_);
            other.inner_ = nullptr;
        }
        return *this;
    }

    void KameleoonClient::ensure_valid() const
    {
        if (inner_ == nullptr)
        {
            throw KameleoonException(ErrorCode::Internal, "KameleoonClient is null");
        }
    }

    void KameleoonClient::release() noexcept
    {
        // Detach from the members first: clearing a handler may synchronously
        // destroy a captured user object whose destructor re-enters this
        // wrapper, and that call must find it already empty. `state` keeps the
        // native handle alive through both clears; the core drops the
        // handlers once no invocation is in progress. Clearing never throws.
        const auto inner = std::exchange(inner_, nullptr);
        const auto state = std::move(callback_state_);
        if (inner == nullptr)
        {
            return;
        }
        detail::set_http_request_handler(inner, {});
        detail::set_datafile_update_handler(inner, {});
    }

    void KameleoonClient::initialize(optional<chrono::milliseconds> timeout) const
    {
        detail::wait_for<void>([&](Completion<void> on_done)
                               { initialize_async(timeout, std::move(on_done)); });
    }

    void KameleoonClient::initialize_async(optional<chrono::milliseconds> timeout, Completion<void> on_done) const
    {
        ensure_valid();
        const auto timeout_millis = checked_timeout_millis(timeout);
        ffi::client__initialize(inner_, timeout_millis, detail::completion(callback_state_, std::move(on_done)));
    }

    bool KameleoonClient::is_ready() const
    {
        ensure_valid();
        return ffi::client__is_ready(inner_);
    }

    string KameleoonClient::get_visitor_code(CookieAccessor &cookies,
                                             optional<string> default_visitor_code) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_cookies = detail::cookie_accessor(cookies);
        const auto raw_default = arena.raw(default_visitor_code);
        const auto visitor_code = detail::call_owned<detail::OwnedString>(
            [&](ffi::OwnedStr *out, ffi::FfiError *error)
            { return ffi::client__get_visitor_code(inner_, raw_cookies, raw_default, out, error); });
        return detail::copy_raw(visitor_code.get());
    }

    void KameleoonClient::set_legal_consent(const string &visitor_code, bool consent, CookieAccessor *cookies) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        optional<ffi::FfiCookieAccessor> raw_cookies;
        if (cookies != nullptr)
        {
            raw_cookies = detail::cookie_accessor(*cookies);
        }
        detail::call_checked(
            [&](ffi::FfiError *error)
            {
                return ffi::client__set_legal_consent(
                    inner_, raw_visitor_code, consent, raw_cookies ? &*raw_cookies : nullptr, error);
            });
    }

    void KameleoonClient::add_data(const string &visitor_code, const vector<Data> &data, bool track) const
    {
        ensure_valid();
        detail::InputArena arena;
        vector<ffi::FfiData> raw_data;
        raw_data.reserve(data.size());
        for (const auto &item : data)
        {
            raw_data.push_back(detail::to_ffi(arena, item));
        }
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_array = arena.array<ffi::FfiArray_FfiData>(std::move(raw_data));
        detail::call_checked([&](ffi::FfiError *error)
                             { return ffi::client__add_data(inner_, raw_visitor_code, track, raw_array, error); });
    }

    void KameleoonClient::flush(const string &visitor_code) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        detail::call_checked([&](ffi::FfiError *error)
                             { return ffi::client__flush(inner_, raw_visitor_code, error); });
    }

    void KameleoonClient::flush_instant(const string &visitor_code) const
    {
        detail::wait_for<void>([&](Completion<void> on_done)
                               { flush_instant_async(visitor_code, std::move(on_done)); });
    }

    void KameleoonClient::flush_instant_async(const string &visitor_code, Completion<void> on_done) const
    {
        ensure_valid();
        // The core copies borrowed inputs before returning from the FFI call,
        // so borrowing from this frame is sufficient.
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        ffi::client__flush_instant(inner_, raw_visitor_code, detail::completion(callback_state_, std::move(on_done)));
    }

    void KameleoonClient::track_conversion(const string &visitor_code,
                                           uint32_t goal_id,
                                           TrackConversionOptions options) const
    {
        ensure_valid();
        detail::InputArena arena;
        vector<ffi::FfiCustomData> metadata;
        metadata.reserve(options.metadata.size());
        for (const auto &item : options.metadata)
        {
            metadata.push_back(detail::to_ffi(arena, item));
        }
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_metadata = arena.array<ffi::FfiArray_FfiCustomData>(std::move(metadata));
        detail::call_checked(
            [&](ffi::FfiError *error)
            {
                return ffi::client__track_conversion(
                    inner_, raw_visitor_code, goal_id, options.revenue, options.negative, raw_metadata, error);
            });
    }

    bool KameleoonClient::is_feature_active(const string &visitor_code,
                                            const string &feature_key,
                                            IsFeatureActiveOptions options) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_feature_key = arena.raw(feature_key);
        bool active = false;
        detail::call_checked(
            [&](ffi::FfiError *error)
            {
                return ffi::client__is_feature_active(
                    inner_, raw_visitor_code, raw_feature_key, options.track, &active, error);
            });
        return active;
    }

    Variation KameleoonClient::get_variation(const string &visitor_code,
                                             const string &feature_key,
                                             GetVariationOptions options) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_feature_key = arena.raw(feature_key);
        const auto variation = detail::call_owned<detail::OwnedVariation>(
            [&](ffi::FfiOwnedVariation *out, ffi::FfiError *error)
            {
                return ffi::client__get_variation(
                    inner_, raw_visitor_code, raw_feature_key, options.track, out, error);
            });
        return detail::copy_variation(variation.get().variation);
    }

    unordered_map<string, Variation> KameleoonClient::get_variations(
        const string &visitor_code,
        GetVariationsOptions options) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto variations = detail::call_owned<detail::OwnedVariations>(
            [&](ffi::FfiVariations *out, ffi::FfiError *error)
            {
                return ffi::client__get_variations(
                    inner_, raw_visitor_code, options.only_active, options.track, out, error);
            });
        return detail::copy_variation_map(variations.get().variations);
    }

    void KameleoonClient::evaluate_audiences(const string &visitor_code) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        detail::call_checked([&](ffi::FfiError *error)
                             { return ffi::client__evaluate_audiences(inner_, raw_visitor_code, error); });
    }

    void KameleoonClient::set_forced_variation(const string &visitor_code,
                                               uint32_t experiment_id,
                                               optional<string> variation_key,
                                               SetForcedVariationOptions options) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_variation_key = arena.raw(variation_key);
        detail::call_checked(
            [&](ffi::FfiError *error)
            {
                return ffi::client__set_forced_variation(
                    inner_, raw_visitor_code, experiment_id, raw_variation_key, options.force_targeting, error);
            });
    }

    string KameleoonClient::get_engine_tracking_code(const string &visitor_code) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto code = detail::call_owned<detail::OwnedString>(
            [&](ffi::OwnedStr *out, ffi::FfiError *error)
            { return ffi::client__get_engine_tracking_code(inner_, raw_visitor_code, out, error); });
        return detail::copy_raw(code.get());
    }

    string KameleoonClient::get_remote_data(const string &key) const
    {
        return detail::wait_for<string>([&](Completion<string> on_done)
                                        { get_remote_data_async(key, std::move(on_done)); });
    }

    void KameleoonClient::get_remote_data_async(const string &key, Completion<string> on_done) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_key = arena.raw(key);
        ffi::client__get_remote_data(inner_, raw_key, detail::completion(callback_state_, std::move(on_done)));
    }

    void KameleoonClient::get_remote_visitor_data(const string &visitor_code, RemoteVisitorDataFilter filter) const
    {
        detail::wait_for<void>([&](Completion<void> on_done)
                               { get_remote_visitor_data_async(visitor_code, filter, std::move(on_done)); });
    }

    void KameleoonClient::get_remote_visitor_data_async(const string &visitor_code,
                                                        RemoteVisitorDataFilter filter,
                                                        Completion<void> on_done) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_filter = detail::to_ffi(filter);
        ffi::client__get_remote_visitor_data(
            inner_, raw_visitor_code, &raw_filter, detail::completion(callback_state_, std::move(on_done)));
    }

    void KameleoonClient::get_visitor_warehouse_audience(const string &visitor_code,
                                                         uint32_t custom_data_index,
                                                         optional<string> warehouse_key) const
    {
        detail::wait_for<void>([&](Completion<void> on_done)
                               { get_visitor_warehouse_audience_async(visitor_code, custom_data_index, warehouse_key, std::move(on_done)); });
    }

    void KameleoonClient::get_visitor_warehouse_audience_async(const string &visitor_code,
                                                               uint32_t custom_data_index,
                                                               optional<string> warehouse_key,
                                                               Completion<void> on_done) const
    {
        ensure_valid();
        detail::InputArena arena;
        const auto raw_visitor_code = arena.raw(visitor_code);
        const auto raw_warehouse_key = arena.raw(warehouse_key);
        ffi::client__get_visitor_warehouse_audience(inner_,
                                                    raw_visitor_code,
                                                    raw_warehouse_key,
                                                    custom_data_index,
                                                    detail::completion(callback_state_, std::move(on_done)));
    }

    DataFile KameleoonClient::get_datafile() const
    {
        ensure_valid();
        const auto datafile = detail::call_owned<detail::OwnedDataFile>(
            [&](ffi::FfiDataFile *out, ffi::FfiError *error)
            { return ffi::client__get_datafile(inner_, out, error); });
        return detail::copy_datafile(datafile.get());
    }

    void KameleoonClient::set_event_handler(EventHandler handler)
    {
        ensure_valid();
        visit(
            detail::Overloaded{
                [&](HttpRequestHandler &&http_request)
                { detail::set_http_request_handler(inner_, std::move(http_request)); },
                [&](DataFileUpdateHandler &&datafile_update)
                { detail::set_datafile_update_handler(inner_, std::move(datafile_update)); },
            },
            std::move(handler.handler_));
    }

} // namespace kameleoon
