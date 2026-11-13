#include "modules/health/HealthRoutes.h"
#include "core/Errors.h"
#include "core/Http.h"

#include <utility>

namespace el::modules::health
{
    namespace ch = el::core::http;

    HealthRoutes::HealthRoutes(std::shared_ptr<db::Database> database,
                               config::Config config,
                               std::string version)
        : database_(std::move(database)), config_(std::move(config)), version_(std::move(version))
    {
    }

    void HealthRoutes::health(const httplib::Request& req, httplib::Response& res) const
    {
        (void)req;
        const std::string requestId = ch::currentRequestId();
        const nlohmann::json data = {
            {"status", "ok"},
            {"version", version_}
        };
        ch::sendJson(res, 200, ch::successEnvelope(data, requestId), requestId, config_);
    }

    void HealthRoutes::ready(const httplib::Request& req, httplib::Response& res) const
    {
        (void)req;
        const std::string requestId = ch::currentRequestId();
        if (!database_ || !database_->isReady())
        {
            ch::sendError(res, 503, "NOT_READY", "database migration has not completed",
                          requestId, config_);
            return;
        }
        const nlohmann::json data = {
            {"status", "ready"},
            {"database", "up"}
        };
        ch::sendJson(res, 200, ch::successEnvelope(data, requestId), requestId, config_);
    }
}
