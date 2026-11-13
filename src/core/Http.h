#pragma once

#include <string>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "config/Config.h"
#include "core/Errors.h"

namespace el::core::http
{
    struct PageQuery
    {
        int page{1};
        int pageSize{20};
    };

    std::string newRequestId();
    void setRequestContext(const std::string& requestId);
    std::string currentRequestId();

    PageQuery parsePage(const httplib::Request& req);

    nlohmann::json successEnvelope(const nlohmann::json& data,
                                   const std::string& requestId,
                                   const nlohmann::json& meta = nlohmann::json::object());

    nlohmann::json errorEnvelope(const std::string& code,
                                 const std::string& message,
                                 const std::string& requestId,
                                 const nlohmann::json& details = nlohmann::json::object());

    void applySecurityHeaders(httplib::Response& res, const config::Config& cfg);

    void sendJson(httplib::Response& res,
                  int status,
                  const nlohmann::json& body,
                  const std::string& requestId,
                  const config::Config& cfg);

    void sendError(httplib::Response& res,
                   const AppError& error,
                   const std::string& requestId,
                   const config::Config& cfg);

    void sendError(httplib::Response& res,
                   int status,
                   const std::string& code,
                   const std::string& message,
                   const std::string& requestId,
                   const config::Config& cfg);

    bool isAuthorized(const httplib::Request& req, const config::Config& cfg);

    nlohmann::json parseJsonBody(const httplib::Request& req, const std::string& requestId);
}
