#pragma once

#include "client_callback_state.hpp"
#include "ffi.hpp"
#include "kameleoon/async.hpp"

#include <exception>
#include <future>
#include <memory>
#include <string>
#include <utility>

// Completions of asynchronous calls.
//
// `completion` boxes the user completion together with a reference to the
// client state (so the native client outlives the call) and returns the
// callback struct the core takes. The core owns the box until it settles the
// call exactly once, on its worker or inline before the export returns; the
// trampoline then takes the box back and must never let an exception cross
// the FFI.
//
// Prepare every other input first: `completion(...)` transfers ownership to
// the core as soon as the export is called, so nothing may throw between the
// two. Pass it in the call with only already-computed locals as the other
// arguments.

namespace kameleoon::detail
{
    template <typename T>
    struct CompletionContext
    {
        std::shared_ptr<ClientCallbackState> client;
        Completion<T> on_done;
    };

    /// An empty completion makes the call fire-and-forget (see `Completion`).
    ffi::FfiVoidCallback completion(const std::shared_ptr<ClientCallbackState> &client, Completion<void> on_done);
    ffi::FfiBytesCallback completion(const std::shared_ptr<ClientCallbackState> &client,
                                     Completion<std::string> on_done);

    // Blocks the calling thread on a completion. The promise is shared with
    // the completion, which may still be unwinding on the worker after
    // `get()` returns here.
    template <typename T, typename Start>
    T wait_for(Start start)
    {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();
        start(Completion<T>([promise](std::exception_ptr error, auto &&...value)
                            {
            if (error)
            {
                promise->set_exception(std::move(error));
            }
            else
            {
                promise->set_value(std::forward<decltype(value)>(value)...);
            } }));
        return future.get();
    }

} // namespace kameleoon::detail
