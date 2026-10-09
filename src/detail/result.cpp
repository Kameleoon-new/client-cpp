#include "result.hpp"

#include <utility>

using namespace std;

namespace kameleoon::detail
{

    namespace
    {

        string copy_bytes(const char *str, uint32_t size)
        {
            if (str == nullptr || size == 0)
            {
                return {};
            }
            return string(str, str + size);
        }

    } // namespace

    string copy_raw(ffi::BorrowedStr value)
    {
        return copy_bytes(value.str, value.size);
    }

    string copy_raw(ffi::OwnedStr value)
    {
        return copy_bytes(value.str, value.size);
    }

    optional<string> copy_optional_raw(ffi::BorrowedStr value)
    {
        if (value.str == nullptr)
        {
            return nullopt;
        }
        return copy_raw(value);
    }

    ErrorCode to_error_code(uint32_t code)
    {
        switch (code)
        {
        case 0:
            return ErrorCode::Initialization;
        case 1:
            return ErrorCode::FeatureNotFound;
        case 2:
            return ErrorCode::FeatureExperimentNotFound;
        case 3:
            return ErrorCode::FeatureVariationNotFound;
        case 4:
            return ErrorCode::FeatureVariableNotFound;
        case 5:
            return ErrorCode::FeatureEnvironmentDisabled;
        case 6:
            return ErrorCode::FeatureRuleNotFound;
        case 7:
            return ErrorCode::FeatureEvaluationBlocked;
        case 10:
            return ErrorCode::VisitorCodeInvalid;
        case 11:
            return ErrorCode::InvalidSimVarCookieFormat;
        case 251:
            return ErrorCode::InvalidArgument;
        case 252:
            return ErrorCode::Timeout;
        case 253:
            return ErrorCode::Network;
        case 254:
            return ErrorCode::Internal;
        default:
            return ErrorCode::Unknown;
        }
    }

    void throw_error(ffi::FfiError error)
    {
        const OwnedError owned(error);
        throw KameleoonException(to_error_code(error.code), copy_raw(error.message));
    }

    exception_ptr error_to_exception(const ffi::FfiBorrowedError *error) noexcept
    {
        if (error == nullptr)
        {
            return nullptr;
        }
        try
        {
            return make_exception_ptr(KameleoonException(to_error_code(error->code), copy_raw(error->message)));
        }
        catch (...)
        {
            return current_exception();
        }
    }

} // namespace kameleoon::detail
