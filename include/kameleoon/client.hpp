#pragma once

#include "kameleoon/async.hpp"
#include "kameleoon/cookie_accessor.hpp"
#include "kameleoon/data/data.hpp"
#include "kameleoon/error.hpp"
#include "kameleoon/events.hpp"
#include "kameleoon/options.hpp"
#include "kameleoon/types/datafile.hpp"
#include "kameleoon/types/remote_visitor_data_filter.hpp"
#include "kameleoon/types/variation.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace kameleoon
{

    namespace detail
    {
        struct ClientCallbackState;
    }

    class KameleoonClient
    {
    public:
        /// Every failure is reported as a `KameleoonException`. String inputs
        /// must be UTF-8; malformed strings, unknown enum values and other
        /// invalid arguments throw with `ErrorCode::InvalidArgument` before
        /// reaching the native core.
        /// Outstanding asynchronous operations keep the native client alive
        /// until their callbacks finish, including after factory::forget().
        ///
        /// The network-bound operations (`initialize`, `flush_instant`,
        /// `get_remote_data`, `get_remote_visitor_data`,
        /// `get_visitor_warehouse_audience`) come in three forms:
        /// - `name(...)`: blocks the calling thread until the operation
        ///   completes. For threads dedicated to waiting: startup, shutdown,
        ///   batch jobs, tests.
        /// - `name_async(..., on_done)`: returns immediately; `on_done` is
        ///   invoked once with the outcome (see `Completion<void>`). For event-loop
        ///   servers: hand the outcome to your loop from the completion. Omit
        ///   `on_done` for fire-and-forget.
        /// - `co_name(...)` (C++20): an `Awaitable` for `co_await`, resumed on
        ///   your loop through `Awaitable::via`.
        /// Never call the blocking form from an SDK completion or event
        /// handler: it waits for the same worker thread and deadlocks.
        ///
        /// Clients are obtained from `KameleoonClientFactory`. A
        /// default-constructed (or moved-from) client holds no native client:
        /// every method except the destructor and move assignment throws
        /// `KameleoonException` with `ErrorCode::Internal`.
        KameleoonClient() = default;
        ~KameleoonClient();

        KameleoonClient(const KameleoonClient &) = delete;
        KameleoonClient &operator=(const KameleoonClient &) = delete;
        KameleoonClient(KameleoonClient &&other) noexcept;
        KameleoonClient &operator=(KameleoonClient &&other) noexcept;

        /// Waits until the client has fetched its configuration.
        ///
        /// When `timeout` is `nullopt`, the `default_timeout_millis` from the
        /// client configuration applies. A provided timeout must be positive;
        /// otherwise `KameleoonException` (`InvalidArgument`) is thrown.
        void initialize(std::optional<std::chrono::milliseconds> timeout = std::nullopt) const;
        void initialize_async(std::optional<std::chrono::milliseconds> timeout = std::nullopt,
                              Completion<void> on_done = {}) const;
        [[nodiscard]] bool is_ready() const;

        [[nodiscard]] std::string get_visitor_code(
            CookieAccessor &cookies,
            std::optional<std::string> default_visitor_code = std::nullopt) const;
        void set_legal_consent(const std::string &visitor_code, bool consent, CookieAccessor *cookies = nullptr) const;

        void add_data(const std::string &visitor_code, const std::vector<Data> &data, bool track = true) const;
        void flush(const std::string &visitor_code) const;
        void flush_instant(const std::string &visitor_code) const;
        void flush_instant_async(const std::string &visitor_code, Completion<void> on_done = {}) const;
        void track_conversion(
            const std::string &visitor_code,
            uint32_t goal_id,
            TrackConversionOptions options = {}) const;

        [[nodiscard]] bool is_feature_active(
            const std::string &visitor_code,
            const std::string &feature_key,
            IsFeatureActiveOptions options = {}) const;
        [[nodiscard]] Variation get_variation(
            const std::string &visitor_code,
            const std::string &feature_key,
            GetVariationOptions options = {}) const;
        [[nodiscard]] std::unordered_map<std::string, Variation> get_variations(
            const std::string &visitor_code,
            GetVariationsOptions options = {}) const;
        void evaluate_audiences(const std::string &visitor_code) const;
        void set_forced_variation(
            const std::string &visitor_code,
            uint32_t experiment_id,
            std::optional<std::string> variation_key = std::nullopt,
            SetForcedVariationOptions options = {}) const;

        [[nodiscard]] std::string get_engine_tracking_code(const std::string &visitor_code) const;
        [[nodiscard]] std::string get_remote_data(const std::string &key) const;
        void get_remote_data_async(const std::string &key, Completion<std::string> on_done = {}) const;
        void get_remote_visitor_data(
            const std::string &visitor_code,
            RemoteVisitorDataFilter filter = {}) const;
        void get_remote_visitor_data_async(
            const std::string &visitor_code,
            RemoteVisitorDataFilter filter = {},
            Completion<void> on_done = {}) const;
        void get_visitor_warehouse_audience(
            const std::string &visitor_code,
            uint32_t custom_data_index,
            std::optional<std::string> warehouse_key = std::nullopt) const;
        void get_visitor_warehouse_audience_async(
            const std::string &visitor_code,
            uint32_t custom_data_index,
            std::optional<std::string> warehouse_key = std::nullopt,
            Completion<void> on_done = {}) const;

#ifdef KAMELEOON_HAS_COROUTINES
        // Header-only so they follow the user's language standard; the
        // library itself is built as C++17. Each starts the `*_async` form
        // when awaited. Arguments are copied into the awaitable.
        [[nodiscard]] Awaitable<void> co_initialize(std::optional<std::chrono::milliseconds> timeout = std::nullopt) const
        {
            return Awaitable<void>([this, timeout](Completion<void> on_done) { initialize_async(timeout, std::move(on_done)); });
        }

        [[nodiscard]] Awaitable<void> co_flush_instant(std::string visitor_code) const
        {
            return Awaitable<void>([this, visitor_code = std::move(visitor_code)](Completion<void> on_done)
                                   { flush_instant_async(visitor_code, std::move(on_done)); });
        }

        [[nodiscard]] Awaitable<std::string> co_get_remote_data(std::string key) const
        {
            return Awaitable<std::string>([this, key = std::move(key)](Completion<std::string> on_done)
                                          { get_remote_data_async(key, std::move(on_done)); });
        }

        [[nodiscard]] Awaitable<void> co_get_remote_visitor_data(std::string visitor_code,
                                                                 RemoteVisitorDataFilter filter = {}) const
        {
            return Awaitable<void>([this, visitor_code = std::move(visitor_code), filter](Completion<void> on_done)
                                   { get_remote_visitor_data_async(visitor_code, filter, std::move(on_done)); });
        }

        [[nodiscard]] Awaitable<void> co_get_visitor_warehouse_audience(
            std::string visitor_code,
            uint32_t custom_data_index,
            std::optional<std::string> warehouse_key = std::nullopt) const
        {
            return Awaitable<void>(
                [this, visitor_code = std::move(visitor_code), custom_data_index, warehouse_key = std::move(warehouse_key)](
                    Completion<void> on_done)
                { get_visitor_warehouse_audience_async(visitor_code, custom_data_index, warehouse_key, std::move(on_done)); });
        }
#endif

        [[nodiscard]] DataFile get_datafile() const;

        /// Registers, or clears, an SDK event handler.
        ///
        /// The `EventHandler` constructor used (`EventHandler::http_request`
        /// or `EventHandler::datafile_update`) selects the `EventType`, so the
        /// handler and the events it receives cannot get out of sync. This
        /// client keeps at most one handler per event type: registering
        /// replaces the previous one, and an empty `std::function` clears it.
        ///
        /// Handlers run synchronously on the SDK's own single worker thread,
        /// so they must not block: time spent in a handler delays data file
        /// polling and tracking flushes. They must be thread-safe and must not
        /// throw (escaping exceptions are caught and discarded). Do not call
        /// blocking SDK operations (initialize, flush_instant, remote-data
        /// methods) inside a handler: they need the same worker to complete.
        /// Use the `*_async` forms and finish the work on an application
        /// thread instead.
        ///
        /// An invocation already in progress may finish after clearing the
        /// handler or destroying the client, and the handler (with its
        /// captures) is destroyed on an SDK thread once it is no longer in
        /// use; keep captured objects alive and thread-safe accordingly. A
        /// handler may clear or replace itself.
        ///
        /// A core client has one handler per event type, as in the other
        /// SDKs: instances created for the same site code and environment
        /// share it, the last one set wins, and destroying any of them
        /// clears it.
        void set_event_handler(EventHandler handler);

    private:
        friend class KameleoonClientFactory;

        explicit KameleoonClient(const void *inner);

        void ensure_valid() const;
        void release() noexcept;

        const void *inner_ = nullptr;
        std::shared_ptr<detail::ClientCallbackState> callback_state_;
    };

} // namespace kameleoon
