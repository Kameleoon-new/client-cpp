#pragma once

namespace kameleoon::detail
{
    // Compares `ffi__abi_version()` with the `FFI_ABI_VERSION` this package was
    // built against and throws `KameleoonException` (`ErrorCode::Internal`) on
    // a mismatch. Every public entry point that passes a struct or callback
    // into libkameleoon_ffi calls it first, so an incompatible library is
    // refused before it can misread a layout. The check is one exported call
    // and is not cached: a mismatch must be reported on every attempt.
    void ensure_abi_compatible();

} // namespace kameleoon::detail
