#pragma once

#include "ffi.hpp"
#include "kameleoon/events.hpp"

// Event handlers of a core client.
//
// The core keeps one handler per event kind and client; setting replaces it,
// an empty handler clears it. The handler is boxed on the heap and the box is
// handed to the core, which owns it until it calls the box's `drop` callback:
// after the handler was replaced or cleared and every invocation in progress
// has returned. So the box, and the user's captures in it, are destroyed on a
// core thread, and nothing here needs to track lifetimes.

namespace kameleoon::detail
{
    /// May throw std::bad_alloc for a non-empty handler; clearing never throws.
    void set_http_request_handler(ffi::StructPtr inner, HttpRequestHandler handler);
    void set_datafile_update_handler(ffi::StructPtr inner, DataFileUpdateHandler handler);

} // namespace kameleoon::detail
