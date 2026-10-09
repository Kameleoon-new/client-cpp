#pragma once

#include "kameleoon/types/variable.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace kameleoon
{
    struct Variation
    {
        std::string key;
        std::string name;
        std::optional<uint32_t> id;
        std::optional<uint32_t> experiment_id;
        std::vector<Variable> variables;

        [[nodiscard]] bool active() const;
        [[nodiscard]] const Variable *get_variable(const std::string &variable_key) const;
    };
} // namespace kameleoon
