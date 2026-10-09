#include "event_handlers.hpp"

#include "result.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

using namespace std;

namespace kameleoon::detail
{
    namespace
    {
        template <typename Handler>
        struct HandlerBox
        {
            Handler handler;
        };

        // Called by the core once it no longer uses the box; see the header.
        template <typename Handler>
        void drop_box(ffi::StructPtr context) noexcept
        {
            delete static_cast<HandlerBox<Handler> *>(const_cast<void *>(context));
        }

        // Runs the boxed handler. User handlers must not throw; escaping
        // exceptions are discarded.
        template <typename Handler, typename Event>
        void invoke(ffi::StructPtr context, const Event &event) noexcept
        {
            try
            {
                static_cast<const HandlerBox<Handler> *>(context)->handler(event);
            }
            catch (...)
            {
                // User handlers must not throw; see set_event_handler docs.
            }
        }

        // Builds the callback struct the core takes: a boxed handler with its
        // trampoline and drop, or all-null to clear.
        template <typename Callback, typename Handler>
        Callback boxed(Handler handler, decltype(Callback::func) trampoline)
        {
            if (!handler)
            {
                return Callback{nullptr, nullptr, nullptr};
            }
            return Callback{new HandlerBox<Handler>{std::move(handler)}, trampoline, drop_box<Handler>};
        }

        void http_request_callback(ffi::StructPtr context, ffi::FfiHttpRequestEvent raw) noexcept
        {
            try
            {
                // The payload borrows from the native caller: copy it before
                // handing it to user code, which may retain the event.
                const auto duration = chrono::milliseconds(static_cast<chrono::milliseconds::rep>(
                    min<uint64_t>(raw.duration_millis,
                                  static_cast<uint64_t>(numeric_limits<chrono::milliseconds::rep>::max()))));
                // The public enums mirror the Rust `#[repr(C)]` enums value for value.
                const auto request_type = static_cast<RequestType>(raw.request_type);
                HttpRequestEvent event;
                if (raw.succeeded)
                {
                    event = HttpRequestSucceeded{request_type, raw.http_status, duration};
                }
                else
                {
                    const auto reason = static_cast<HttpRequestFailureReason>(raw.failure_reason);
                    HttpRequestFailure failure{reason, nullopt, nullopt};
                    if (reason == HttpRequestFailureReason::HttpStatus)
                    {
                        failure.http_status = raw.http_status;
                    }
                    if (reason == HttpRequestFailureReason::Error)
                    {
                        failure.cause = copy_optional_raw(raw.failure_cause);
                    }
                    event = HttpRequestFailed{request_type, std::move(failure), duration};
                }
                invoke<HttpRequestHandler>(context, event);
            }
            catch (...)
            {
                // Allocation failure while copying the payload: drop the event.
            }
        }

        void datafile_update_callback(ffi::StructPtr context, ffi::FfiDataFileUpdateEvent raw) noexcept
        {
            invoke<DataFileUpdateHandler>(
                context, DataFileUpdateEvent{static_cast<DataFileUpdateSource>(raw.source), raw.date_modified});
        }

    } // namespace

    void set_http_request_handler(ffi::StructPtr inner, HttpRequestHandler handler)
    {
        ffi::client__set_http_request_handler(
            inner, boxed<ffi::FfiHttpRequestEventCallback>(std::move(handler), http_request_callback));
    }

    void set_datafile_update_handler(ffi::StructPtr inner, DataFileUpdateHandler handler)
    {
        ffi::client__set_datafile_update_handler(
            inner, boxed<ffi::FfiDataFileUpdateEventCallback>(std::move(handler), datafile_update_callback));
    }

} // namespace kameleoon::detail
