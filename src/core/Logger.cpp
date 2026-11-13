#include "core/Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace el::core::logger
{
    namespace
    {
        std::shared_ptr<spdlog::logger> g_logger;

        spdlog::level::level_enum toSpdlog(const std::string& level)
        {
            if (level == "trace") return spdlog::level::trace;
            if (level == "debug") return spdlog::level::debug;
            if (level == "warn" || level == "warning") return spdlog::level::warn;
            if (level == "error") return spdlog::level::err;
            if (level == "critical") return spdlog::level::critical;
            return spdlog::level::info;
        }
    }

    std::string nowIso8601Utc()
    {
        const auto now = std::chrono::system_clock::now();
        const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
        std::tm tmUtc{};
#ifdef _WIN32
        gmtime_s(&tmUtc, &seconds);
#else
        gmtime_r(&seconds, &tmUtc);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tmUtc, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }

    void init(const std::string& level)
    {
        g_logger = spdlog::stderr_color_mt("el-backend");
        g_logger->set_pattern("%v");
        g_logger->set_level(toSpdlog(level));
        g_logger->flush_on(spdlog::level::warn);
        spdlog::set_default_logger(g_logger);
    }

    void write(const std::string& level,
               const std::string& event,
               const std::string& requestId,
               nlohmann::json fields)
    {
        nlohmann::json record = {
            {"ts", nowIso8601Utc()},
            {"level", level},
            {"event", event},
            {"requestId", requestId}
        };
        if (fields.is_object() && !fields.empty())
        {
            record["fields"] = std::move(fields);
        }

        const auto severity = toSpdlog(level);
        if (g_logger)
        {
            g_logger->log(severity, "{}", record.dump());
        }
    }
}
