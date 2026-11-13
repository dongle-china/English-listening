#include "config/Config.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace el::config
{
    namespace
    {
        std::string envOr(const char* key, std::string fallback)
        {
            const char* value = std::getenv(key);
            return (value != nullptr && *value != '\0') ? std::string(value) : std::move(fallback);
        }

        std::string makeToken()
        {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<int> dist(0, 255);
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (int i = 0; i < 32; ++i)
            {
                oss << std::setw(2) << dist(gen);
            }
            return oss.str();
        }

        int parsePort(const std::string& raw)
        {
            try
            {
                std::size_t consumed = 0;
                const long value = std::stol(raw, &consumed);
                if (consumed != raw.size() || value < 0 || value > 65535)
                {
                    throw std::runtime_error("EL_PORT must be an integer in [0, 65535]");
                }
                return static_cast<int>(value);
            }
            catch (const std::exception&)
            {
                throw std::runtime_error("EL_PORT must be an integer in [0, 65535]");
            }
        }
    }

    bool Config::validate(std::string& error) const
    {
        if (host != "127.0.0.1" && host != "localhost" && host != "::1")
        {
            error = "EL_HOST must be a loopback address (local service only)";
            return false;
        }
        if (corsOrigin.empty() || corsOrigin == "*")
        {
            error = "EL_CORS_ORIGIN must be an explicit origin, wildcard is forbidden";
            return false;
        }
        if (dbPath.empty())
        {
            error = "EL_DB_PATH must not be empty";
            return false;
        }
        if (token.size() < 16)
        {
            error = "EL_TOKEN must contain at least 16 characters";
            return false;
        }
        if (logLevel.empty())
        {
            error = "EL_LOG_LEVEL must not be empty";
            return false;
        }
        return true;
    }

    Config load()
    {
        Config cfg;
        cfg.host = envOr("EL_HOST", "127.0.0.1");
        cfg.port = static_cast<std::uint16_t>(parsePort(envOr("EL_PORT", "0")));
        cfg.dbPath = envOr("EL_DB_PATH", "./data/english_listening.db");
        cfg.logLevel = envOr("EL_LOG_LEVEL", "info");
        cfg.corsOrigin = envOr("EL_CORS_ORIGIN", "http://localhost:5173");
        cfg.token = envOr("EL_TOKEN", makeToken());

        std::error_code ec;
        const auto parent = std::filesystem::path(cfg.dbPath).parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent, ec);
            if (ec)
            {
                throw std::runtime_error("cannot create database directory: " + ec.message());
            }
        }

        std::string error;
        if (!cfg.validate(error))
        {
            throw std::runtime_error("invalid configuration: " + error);
        }
        return cfg;
    }
}
