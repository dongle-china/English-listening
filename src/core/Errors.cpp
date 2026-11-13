#include "core/Errors.h"

namespace el::core
{
    const char* errorCodeName(ErrorCode code) noexcept
    {
        switch (code)
        {
            case ErrorCode::InvalidArgument: return "INVALID_ARGUMENT";
            case ErrorCode::Unauthorized: return "UNAUTHORIZED";
            case ErrorCode::NotFound: return "NOT_FOUND";
            case ErrorCode::Conflict: return "CONFLICT";
            case ErrorCode::ValidationFailed: return "VALIDATION_FAILED";
            case ErrorCode::RateLimited: return "RATE_LIMITED";
            case ErrorCode::Internal: return "INTERNAL_ERROR";
            case ErrorCode::NotReady: return "NOT_READY";
        }
        return "INTERNAL_ERROR";
    }

    AppError::AppError(ErrorCode code, int status, std::string message)
        : std::runtime_error(std::move(message)), code_(code), status_(status)
    {
    }
}
