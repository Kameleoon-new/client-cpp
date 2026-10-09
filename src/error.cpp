#include "kameleoon/error.hpp"

#include <utility>

using namespace std;

namespace kameleoon
{

    KameleoonException::KameleoonException(ErrorCode code, string message)
        : runtime_error(std::move(message)), code_(code) {}

    ErrorCode KameleoonException::code() const noexcept
    {
        return code_;
    }

} // namespace kameleoon
