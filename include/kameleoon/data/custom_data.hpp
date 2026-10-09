#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kameleoon
{
    struct CustomData
    {
        /// `id` of a custom data identified by `name`; the SDK resolves the
        /// index from the data file.
        static constexpr uint32_t UNDEFINED_ID = UINT32_MAX;

        CustomData(uint32_t id, std::vector<std::string> values,
                   bool overwrite = true)
            : id(id), values(std::move(values)), overwrite(overwrite) {}

        CustomData(std::string name, std::vector<std::string> values,
                   bool overwrite = true)
            : id(UNDEFINED_ID), name(std::move(name)), values(std::move(values)),
              overwrite(overwrite) {}

        /// Takes precedence over `name` unless it is `UNDEFINED_ID`.
        uint32_t id;
        std::optional<std::string> name;
        std::vector<std::string> values;
        /// Replace the visitor's previous values for this custom data instead of
        /// merging.
        bool overwrite = true;
    };
} // namespace kameleoon
