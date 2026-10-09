#include "abi.hpp"

#include "ffi.hpp"
#include "kameleoon/error.hpp"

#include <cstdint>
#include <string>

namespace kameleoon::detail
{
    void ensure_abi_compatible()
    {
        const uint32_t runtime_version = ffi::ffi__abi_version();
        if (runtime_version != FFI_ABI_VERSION)
        {
            throw KameleoonException(ErrorCode::Internal,
                                     "libkameleoon_ffi ABI version " + std::to_string(runtime_version) +
                                         " does not match the version this SDK was built for (" +
                                         std::to_string(FFI_ABI_VERSION) + ")");
        }
    }

} // namespace kameleoon::detail
