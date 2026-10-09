#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace kameleoon
{
    /// SDK observability events, registered per type with
    /// `KameleoonClient::set_event_handler`.
    enum class EventType : uint32_t
    {
        /// An HTTP request attempt performed by the SDK completed or failed.
        HttpRequest = 0,
        /// The data file (configuration) was updated by polling or streaming.
        DataFileUpdate = 1,
    };

    /// The type of an HTTP request performed by the SDK.
    enum class RequestType : uint32_t
    {
        DataFile = 0,
        Tracking = 1,
        RemoteVisitorData = 2,
        RemoteData = 3,
        AccessToken = 4,
    };

    enum class HttpRequestFailureReason : uint32_t
    {
        /// The request completed with an unexpected HTTP status (see `http_status`).
        HttpStatus = 0,
        /// The request failed with an error, described by `cause`.
        Error = 1,
        /// The request was cancelled, for example because it exceeded its timeout.
        Cancelled = 2,
    };

    enum class DataFileUpdateSource : uint32_t
    {
        Polling = 0,
        Streaming = 1,
    };

    struct HttpRequestFailure
    {
        HttpRequestFailureReason reason;
        /// Set only for `HttpRequestFailureReason::HttpStatus`.
        std::optional<uint16_t> http_status;
        /// Set only for `HttpRequestFailureReason::Error`.
        std::optional<std::string> cause;
    };

    /// An SDK HTTP request attempt which completed successfully.
    struct HttpRequestSucceeded
    {
        RequestType request_type;
        uint16_t http_status;
        std::chrono::milliseconds duration;
    };

    /// An SDK HTTP request attempt which failed.
    struct HttpRequestFailed
    {
        RequestType request_type;
        HttpRequestFailure failure;
        std::chrono::milliseconds duration;
    };

    /// Delivered once per actual HTTP request attempt, retries included, so a
    /// single logical operation may produce several events.
    using HttpRequestEvent = std::variant<HttpRequestSucceeded, HttpRequestFailed>;

    /// An update of the SDK data file (configuration).
    struct DataFileUpdateEvent
    {
        DataFileUpdateSource source;
        /// Modification date of the data file, in milliseconds since the Unix epoch.
        uint64_t date_modified;
    };

    using HttpRequestHandler = std::function<void(const HttpRequestEvent &event)>;
    using DataFileUpdateHandler = std::function<void(const DataFileUpdateEvent &event)>;

    /// The handler to register for a single `EventType`, built with one of the
    /// static constructors so the handler and the events it receives cannot get
    /// out of sync. An empty `std::function` clears the handler for that type.
    ///
    ///     client.set_event_handler(EventHandler::http_request([](const HttpRequestEvent &event) {
    ///         if (const auto *failed = std::get_if<HttpRequestFailed>(&event)) {
    ///             log_warning(failed->request_type, failed->failure.reason);
    ///         }
    ///     }));
    ///
    ///     // Stop listening.
    ///     client.set_event_handler(EventHandler::http_request({}));
    class EventHandler
    {
    public:
        static EventHandler http_request(HttpRequestHandler handler);
        static EventHandler datafile_update(DataFileUpdateHandler handler);

    private:
        friend class KameleoonClient;

        using Handler = std::variant<HttpRequestHandler, DataFileUpdateHandler>;

        explicit EventHandler(Handler handler);

        Handler handler_;
    };
} // namespace kameleoon
