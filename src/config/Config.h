#pragma once

#include <cstdint>
#include <string>

namespace el::config
{
    struct Config
    {
        std::string host{"127.0.0.1"};
        std::uint16_t port{0};
        std::string dbPath;
        std::string logDir;
        std::string token;
        std::string corsOrigin;
        std::string logLevel{"info"};
        std::string version{"0.1.0"};

        bool validate(std::string& error) const;
    };

    Config load();
}
