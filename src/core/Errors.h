#pragma once

#include <stdexcept>
#include <string>

namespace el::core
{
    enum class ErrorCode
    {
        InvalidArgument,
        Unauthorized,
        NotFound,
        Conflict,
        ValidationFailed,
        RateLimited,
        Internal,
        NotReady
    };

    const char* errorCodeName(ErrorCode code) noexcept;

    class AppError : public std::runtime_error
    {
    public:
        AppError(ErrorCode code, int status, std::string message);

        ErrorCode code() const noexcept { return code_; }
        int status() const noexcept { return status_; }

    private:
        ErrorCode code_;
        int status_;
    };
}
