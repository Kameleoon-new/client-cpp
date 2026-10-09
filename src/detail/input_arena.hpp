#pragma once

#include "ffi.hpp"
#include "kameleoon/error.hpp"

#include <cstdint>
#include <memory>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace kameleoon::detail
{
    inline uint32_t checked_ffi_size(std::size_t size)
    {
        if (size > std::numeric_limits<uint32_t>::max())
        {
            throw KameleoonException(ErrorCode::InvalidArgument, "Kameleoon input exceeds the native 32-bit length limit");
        }
        return static_cast<uint32_t>(size);
    }

    // Borrows `value` as a Rust &str. Throws KameleoonException
    // (InvalidArgument) on invalid UTF-8 or a size beyond the 32-bit ABI.
    ffi::BorrowedStr checked_utf8(std::string_view value);

    class InputArena
    {
    public:
        ffi::BorrowedStr raw(const std::string &value);
        ffi::BorrowedStr raw(std::string &&value) = delete;
        ffi::BorrowedStr raw(const std::string &&value) = delete;
        ffi::BorrowedStr raw(const char *value);
        ffi::BorrowedStr raw(const std::optional<std::string> &value);
        ffi::BorrowedStr raw(std::optional<std::string> &&value) = delete;
        ffi::BorrowedStr raw(const std::optional<std::string> &&value) = delete;

        // Borrows `values` as the cbindgen array type `Array` (FfiArray_<T> in
        // the generated header) for the lifetime of this arena.
        template <typename Array, typename T>
        Array array(std::vector<T> values)
        {
            if (values.empty())
            {
                return Array{nullptr, 0};
            }

            const auto size = checked_ffi_size(values.size());
            auto holder = std::make_unique<ArrayHolder<T>>(std::move(values));
            auto result = Array{holder->values.data(), size};
            arrays_.push_back(std::move(holder));
            return result;
        }

    private:
        struct ArrayHolderBase
        {
            virtual ~ArrayHolderBase() = default;
        };

        template <typename T>
        struct ArrayHolder final : ArrayHolderBase
        {
            explicit ArrayHolder(std::vector<T> held) : values(std::move(held)) {}
            std::vector<T> values;
        };

        std::vector<std::unique_ptr<ArrayHolderBase>> arrays_;
    };

} // namespace kameleoon::detail
