#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace el::core::logger
{
    void init(const std::string& level);

    void write(const std::string& level,
               const std::string& event,
               const std::string& requestId,
               nlohmann::json fields = nlohmann::json::object());

    std::string nowIso8601Utc();
}
