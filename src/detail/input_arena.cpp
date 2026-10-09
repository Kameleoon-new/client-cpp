#include "input_arena.hpp"

#include <cstring>
#include <string_view>

using namespace std;

namespace kameleoon::detail
{
    // Rust's borrowed-string ABI requires valid UTF-8 (the core uses
    // from_utf8_unchecked). std::string has no such invariant, so reject
    // invalid input before it can become a Rust &str.
    ffi::BorrowedStr checked_utf8(std::string_view value)
    {
        const auto size = checked_ffi_size(value.size());
        std::size_t i = 0;
        while (i < value.size())
        {
            // Fast path: skip 8 bytes at once while none has the high bit set
            // (pure ASCII, which is what keys, visitor codes and most cookie
            // values are). memcpy avoids an unaligned load.
            if (value.size() - i >= 8)
            {
                uint64_t word;
                std::memcpy(&word, value.data() + i, sizeof word);
                if ((word & 0x8080808080808080ULL) == 0)
                {
                    i += 8;
                    continue;
                }
            }
            const auto first = static_cast<unsigned char>(value[i++]);
            if (first < 0x80)
                continue;
            unsigned remaining;
            uint32_t codepoint;
            uint32_t minimum;
            if (first >= 0xc2 && first <= 0xdf)
            {
                remaining = 1;
                codepoint = first & 0x1f;
                minimum = 0x80;
            }
            else if (first >= 0xe0 && first <= 0xef)
            {
                remaining = 2;
                codepoint = first & 0x0f;
                minimum = 0x800;
            }
            else if (first >= 0xf0 && first <= 0xf4)
            {
                remaining = 3;
                codepoint = first & 0x07;
                minimum = 0x10000;
            }
            else
            {
                throw KameleoonException(ErrorCode::InvalidArgument, "Kameleoon string inputs must be valid UTF-8");
            }
            if (remaining > value.size() - i)
                throw KameleoonException(ErrorCode::InvalidArgument, "Kameleoon string inputs must be valid UTF-8");
            while (remaining--)
            {
                const auto next = static_cast<unsigned char>(value[i++]);
                if ((next & 0xc0) != 0x80)
                    throw KameleoonException(ErrorCode::InvalidArgument, "Kameleoon string inputs must be valid UTF-8");
                codepoint = (codepoint << 6) | (next & 0x3f);
            }
            if (codepoint < minimum || codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff))
                throw KameleoonException(ErrorCode::InvalidArgument, "Kameleoon string inputs must be valid UTF-8");
        }
        return ffi::BorrowedStr{value.data(), size};
    }

    ffi::BorrowedStr InputArena::raw(const string &value)
    {
        return checked_utf8(value);
    }

    ffi::BorrowedStr InputArena::raw(const char *value)
    {
        if (value == nullptr)
        {
            return ffi::BorrowedStr{nullptr, 0};
        }
        return checked_utf8(value);
    }

    ffi::BorrowedStr InputArena::raw(const optional<string> &value)
    {
        if (!value.has_value())
        {
            return ffi::BorrowedStr{nullptr, 0};
        }
        return raw(*value);
    }

} // namespace kameleoon::detail
