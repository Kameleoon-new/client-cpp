#include "kameleoon/types/variation.hpp"

#include <algorithm>

using namespace std;

namespace kameleoon
{
    bool Variation::active() const
    {
        return key != "off";
    }

    const Variable *Variation::get_variable(const string &variable_key) const
    {
        auto it = find_if(variables.begin(), variables.end(), [&](const Variable &variable)
                          { return variable.key == variable_key; });
        return it == variables.end() ? nullptr : &*it;
    }
} // namespace kameleoon
