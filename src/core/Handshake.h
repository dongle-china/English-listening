#pragma once

#include <cstdint>
#include <string>

namespace el::core::handshake
{
    struct Ready
    {
        std::string host;
        std::uint16_t port{0};
        std::uint16_t wsPort{0};
        std::string token;
        std::string version;
        int pid{0};
    };

    void emit(const Ready& ready);
}
