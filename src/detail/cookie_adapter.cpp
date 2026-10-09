#include "cookie_adapter.hpp"

#include "result.hpp"
#include "input_arena.hpp"

#include <cstring>

using namespace std;

namespace kameleoon::detail
{
    namespace
    {

        // These callbacks are invoked from Rust through `extern "C"` function
        // pointers. A C++ exception must never unwind across that boundary (that is
        // undefined behavior), so both callbacks are `noexcept` and swallow anything
        // thrown by user-provided `CookieAccessor` overrides or by allocations.

        void cookie_set(ffi::StructPtr context,
                        ffi::BorrowedStr key,
                        ffi::BorrowedStr value,
                        uint32_t max_age,
                        ffi::BorrowedStr top_level_domain) noexcept
        {
            try
            {
                auto *accessor = static_cast<CookieAccessor *>(const_cast<void *>(context));
                accessor->set(copy_raw(key), copy_raw(value), max_age, copy_optional_raw(top_level_domain));
            }
            catch (...)
            {
                // User cookie writers must not throw; see CookieAccessor docs.
            }
        }

        ffi::ForeignOwnedStr cookie_get(ffi::StructPtr context, ffi::BorrowedStr key) noexcept
        {
            try
            {
                auto *accessor = static_cast<CookieAccessor *>(const_cast<void *>(context));
                auto value = accessor->get(copy_raw(key));
                if (!value.has_value())
                {
                    return ffi::ForeignOwnedStr{nullptr, 0};
                }

                // The core reads this as a &str, so a non-UTF-8 value must not cross
                // the boundary; the throw below is caught and reported as "absent".
                const auto size = checked_utf8(*value).size;
                auto *buffer = new char[value->size()];
                if (!value->empty())
                {
                    memcpy(buffer, value->data(), value->size());
                }
                return ffi::ForeignOwnedStr{buffer, size};
            }
            catch (...)
            {
                // A throwing getter behaves as if the cookie were absent.
                return ffi::ForeignOwnedStr{nullptr, 0};
            }
        }

        void cookie_free_str(ffi::ForeignOwnedStr value) noexcept
        {
            delete[] const_cast<char *>(value.str);
        }

    } // namespace

    ffi::FfiCookieAccessor cookie_accessor(CookieAccessor &accessor)
    {
        return ffi::FfiCookieAccessor{&accessor, cookie_set, cookie_get, cookie_free_str};
    }

} // namespace kameleoon::detail
