#include "core/Http.h"
#include "core/Logger.h"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

namespace el::core::http
{
    namespace
    {
        int parseInt(const std::string& raw, int fallback, int lo, int hi)
        {
            if (raw.empty()) return fallback;
            try
            {
                std::size_t consumed = 0;
                const long value = std::stol(raw, &consumed);
                if (consumed != raw.size()) return fallback;
                return static_cast<int>(std::clamp<long>(value, lo, hi));
            }
            catch (const std::exception&)
            {
                return fallback;
            }
        }

        std::string headerValue(const httplib::Request& req, const char* name)
        {
            const auto it = req.headers.find(name);
            return it == req.headers.end() ? std::string{} : it->second;
        }

        void setUniqueHeader(httplib::Response& res, const std::string& key, const std::string& value)
        {
            res.headers.erase(key);
            res.set_header(key, value);
        }

        thread_local std::string t_requestId;
    }

    std::string newRequestId()
    {
        static thread_local std::mt19937_64 gen(std::random_device{}());
        std::uniform_int_distribution<std::uint64_t> dist;
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << dist(gen)
            << std::setw(16) << dist(gen);
        return oss.str();
    }

    void setRequestContext(const std::string& requestId)
    {
        t_requestId = requestId;
    }

    std::string currentRequestId()
    {
        if (t_requestId.empty()) t_requestId = newRequestId();
        return t_requestId;
    }

    PageQuery parsePage(const httplib::Request& req)
    {
        PageQuery query;
        if (req.has_param("page")) query.page = parseInt(req.get_param_value("page"), 1, 1, 1000000);
        if (req.has_param("pageSize")) query.pageSize = parseInt(req.get_param_value("pageSize"), 20, 1, 100);
        return query;
    }

    nlohmann::json successEnvelope(const nlohmann::json& data,
                                   const std::string& requestId,
                                   const nlohmann::json& meta)
    {
        nlohmann::json body = {
            {"ok", true},
            {"data", data},
            {"requestId", requestId},
            {"ts", logger::nowIso8601Utc()}
        };
        if (meta.is_object() && !meta.empty()) body["meta"] = meta;
        return body;
    }

    nlohmann::json errorEnvelope(const std::string& code,
                                 const std::string& message,
                                 const std::string& requestId,
                                 const nlohmann::json& details)
    {
        nlohmann::json error = {{"code", code}, {"message", message}};
        if (details.is_object() && !details.empty()) error["details"] = details;
        return {
            {"ok", false},
            {"error", error},
            {"requestId", requestId},
            {"ts", logger::nowIso8601Utc()}
        };
    }

    void applySecurityHeaders(httplib::Response& res, const config::Config& cfg)
    {
        setUniqueHeader(res, "Access-Control-Allow-Origin", cfg.corsOrigin);
        setUniqueHeader(res, "Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        setUniqueHeader(res, "Access-Control-Allow-Headers",
                        "Authorization, Content-Type, X-Request-Id");
        setUniqueHeader(res, "Access-Control-Max-Age", "600");
        setUniqueHeader(res, "Vary", "Origin");
        setUniqueHeader(res, "X-Content-Type-Options", "nosniff");
        setUniqueHeader(res, "X-Frame-Options", "DENY");
        setUniqueHeader(res, "Referrer-Policy", "no-referrer");
        setUniqueHeader(res, "Cache-Control", "no-store");
    }

    void sendJson(httplib::Response& res,
                  int status,
                  const nlohmann::json& body,
                  const std::string& requestId,
                  const config::Config& cfg)
    {
        applySecurityHeaders(res, cfg);
        setUniqueHeader(res, "X-Request-Id", requestId);
        res.status = status;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    }

    void sendError(httplib::Response& res,
                   int status,
                   const std::string& code,
                   const std::string& message,
                   const std::string& requestId,
                   const config::Config& cfg)
    {
        sendJson(res, status, errorEnvelope(code, message, requestId), requestId, cfg);
    }

    void sendError(httplib::Response& res,
                   const AppError& error,
                   const std::string& requestId,
                   const config::Config& cfg)
    {
        sendError(res, error.status(), errorCodeName(error.code()), error.what(), requestId, cfg);
    }

    bool isAuthorized(const httplib::Request& req, const config::Config& cfg)
    {
        const std::string auth = headerValue(req, "Authorization");
        const std::string expected = "Bearer " + cfg.token;
        if (auth == expected) return true;
        const std::string headerToken = headerValue(req, "X-EL-Token");
        return headerToken == cfg.token;
    }

    nlohmann::json parseJsonBody(const httplib::Request& req, const std::string& requestId)
    {
        if (req.body.empty())
        {
            throw AppError(ErrorCode::InvalidArgument, 400, "request body must not be empty");
        }
        auto parsed = nlohmann::json::parse(req.body, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object())
        {
            throw AppError(ErrorCode::ValidationFailed, 422, "request body must be a JSON object");
        }
        (void)requestId;
        return parsed;
    }
}
