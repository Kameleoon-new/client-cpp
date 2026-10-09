#pragma once

#include "ffi.hpp"

namespace kameleoon::detail
{

    // Owns one native handle of the core client. Shared by the wrapper and every
    // outstanding completion, so the native runtime stays alive until its
    // callbacks have completed, even after the wrapper is destroyed or the
    // factory forgets the client.
    struct ClientCallbackState
    {
        const void *inner = nullptr;
        ~ClientCallbackState()
        {
            if (inner)
                ffi::client__free(inner);
        }
    };

} // namespace kameleoon::detail
