#include "modules/wordbank/WordBankController.h"
#include "core/Errors.h"
#include "core/Http.h"
#include "core/Logger.h"

#include <utility>

namespace el::modules::wordbank
{
    namespace ch = el::core::http;

    WordBankController::WordBankController(std::shared_ptr<WordBankService> service, config::Config config)
        : service_(std::move(service)), config_(std::move(config))
    {
    }

    void WordBankController::list(const httplib::Request& req, httplib::Response& res) const
    {
        const std::string requestId = ch::currentRequestId();
        try
        {
            const auto query = ch::parsePage(req);
            const PageResult result = service_->list({query.page, query.pageSize});

            nlohmann::json items = nlohmann::json::array();
            for (const auto& record : result.items)
            {
                items.push_back(record.toJson());
            }

            const nlohmann::json data = {{"items", items}};
            const nlohmann::json meta = {
                {"page", query.page},
                {"pageSize", query.pageSize},
                {"total", result.total}
            };
            ch::sendJson(res, 200, ch::successEnvelope(data, requestId, meta), requestId, config_);
            core::logger::write("info", "wordbank.list", requestId,
                                {{"count", result.items.size()}, {"total", result.total}});
        }
        catch (const core::AppError& error)
        {
            ch::sendError(res, error, requestId, config_);
        }
    }

    void WordBankController::create(const httplib::Request& req, httplib::Response& res) const
    {
        const std::string requestId = ch::currentRequestId();
        try
        {
            const auto body = ch::parseJsonBody(req, requestId);
            if (!body.contains("name") || !body["name"].is_string())
            {
                throw core::AppError(core::ErrorCode::ValidationFailed, 422, "name must be a string");
            }

            CreateWordBankInput input;
            input.name = body["name"].get<std::string>();
            if (body.contains("description") && body["description"].is_string())
            {
                input.description = body["description"].get<std::string>();
            }

            const WordBankRecord record = service_->create(input);
            ch::sendJson(res, 201, ch::successEnvelope(record.toJson(), requestId), requestId, config_);
            core::logger::write("info", "wordbank.create", requestId, {{"id", record.id}});
        }
        catch (const core::AppError& error)
        {
            ch::sendError(res, error, requestId, config_);
        }
    }

    void WordBankController::get(const httplib::Request& req, httplib::Response& res,
                                 const std::string& id) const
    {
        (void)req;
        const std::string requestId = ch::currentRequestId();
        try
        {
            const WordBankRecord record = service_->getById(id);
            ch::sendJson(res, 200, ch::successEnvelope(record.toJson(), requestId), requestId, config_);
        }
        catch (const core::AppError& error)
        {
            ch::sendError(res, error, requestId, config_);
        }
    }

    void WordBankController::remove(const httplib::Request& req, httplib::Response& res,
                                    const std::string& id) const
    {
        (void)req;
        const std::string requestId = ch::currentRequestId();
        try
        {
            service_->remove(id);
            ch::sendJson(res, 200, ch::successEnvelope(nlohmann::json{{"id", id}}, requestId),
                         requestId, config_);
            core::logger::write("info", "wordbank.remove", requestId, {{"id", id}});
        }
        catch (const core::AppError& error)
        {
            ch::sendError(res, error, requestId, config_);
        }
    }
}
