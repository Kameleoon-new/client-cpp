#pragma once

#include "ffi.hpp"
#include "kameleoon/error.hpp"

#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>

namespace kameleoon::detail
{
    constexpr uint32_t NO_U32 = 0xFFFFFFFFU;

    std::string copy_raw(ffi::BorrowedStr value);
    std::string copy_raw(ffi::OwnedStr value);
    std::optional<std::string> copy_optional_raw(ffi::BorrowedStr value);
    ErrorCode to_error_code(uint32_t code);

    /// Converts an error written by a fallible export into an exception. Takes
    /// ownership of `error` and releases it.
    [[noreturn]] void throw_error(ffi::FfiError error);

    /// Converts a borrowed callback error into an exception_ptr, copying the
    /// message before the callback returns; null when `error` is null
    /// (success). Never throws: a failure to build the exception (allocation)
    /// is returned as that exception instead.
    std::exception_ptr error_to_exception(const ffi::FfiBorrowedError *error) noexcept;

    /// Owns a value written to an `out` parameter and releases it with `Free`
    /// on scope exit, so conversions that throw cannot leak Rust memory.
    template <typename T, void (*Free)(T)>
    class Owned
    {
    public:
        Owned() = default;
        explicit Owned(T value) noexcept : value_(value), owns_(true) {}
        ~Owned()
        {
            if (owns_)
            {
                Free(value_);
            }
        }

        Owned(const Owned &) = delete;
        Owned &operator=(const Owned &) = delete;
        Owned(Owned &&other) noexcept : value_(other.value_), owns_(std::exchange(other.owns_, false)) {}
        Owned &operator=(Owned &&) = delete;

        const T &get() const noexcept { return value_; }
        /// Destination for an `out` parameter; call `adopt()` once the export
        /// reported success and the value must be released.
        T *out() noexcept { return &value_; }
        void adopt() noexcept { owns_ = true; }

    private:
        T value_{};
        bool owns_ = false;
    };

    using OwnedError = Owned<ffi::FfiError, ffi::error__free>;
    using OwnedString = Owned<ffi::OwnedStr, ffi::string__free>;
    using OwnedVariation = Owned<ffi::FfiOwnedVariation, ffi::variation__free>;
    using OwnedVariations = Owned<ffi::FfiVariations, ffi::variations__free>;
    using OwnedDataFile = Owned<ffi::FfiDataFile, ffi::datafile__free>;

    /// Runs `call(error)`, where `call` is a fallible export returning `bool`,
    /// and throws `KameleoonException` when it reports a failure.
    template <typename Fn>
    void call_checked(Fn &&call)
    {
        ffi::FfiError error{};
        if (!call(&error))
        {
            throw_error(error);
        }
    }

    /// Like `call_checked` for exports with an `out` parameter: `call(out, error)`
    /// writes into `owned`, which then owns the payload.
    template <typename Owner, typename Fn>
    Owner call_owned(Fn &&call)
    {
        Owner owned;
        ffi::FfiError error{};
        if (!call(owned.out(), &error))
        {
            throw_error(error);
        }
        owned.adopt();
        return owned;
    }
} // namespace kameleoon::detail
