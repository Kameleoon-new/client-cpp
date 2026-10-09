#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace kameleoon
{

    enum class ErrorCode : uint32_t
    {
        Initialization = 0,
        FeatureNotFound = 1,
        FeatureExperimentNotFound = 2,
        FeatureVariationNotFound = 3,
        FeatureVariableNotFound = 4,
        FeatureEnvironmentDisabled = 5,
        FeatureRuleNotFound = 6,
        FeatureEvaluationBlocked = 7,
        VisitorCodeInvalid = 10,
        InvalidSimVarCookieFormat = 11,
        InvalidArgument = 251,
        Timeout = 252,
        Network = 253,
        Internal = 254,
        Unknown = 255,
    };

    class KameleoonException : public std::runtime_error
    {
    public:
        KameleoonException(ErrorCode code, std::string message);

        [[nodiscard]] ErrorCode code() const noexcept;

    private:
        ErrorCode code_;
    };

} // namespace kameleoon
