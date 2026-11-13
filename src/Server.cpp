#include "Server.h"
#include "core/Errors.h"
#include "core/Handshake.h"
#include "core/Http.h"
#include "core/Logger.h"

#include <chrono>
#include <functional>
#include <utility>

namespace el
{
    namespace ch = core::http;

    Server::Server(config::Config config, std::shared_ptr<db::Database> database)
        : config_(std::move(config)), database_(std::move(database))
    {
        hub_ = std::make_shared<ws::ProgressHub>(config_.token);
        tasks_ = std::make_shared<modules::tasks::TaskService>(hub_);
        health_ = std::make_unique<modules::health::HealthRoutes>(database_, config_, config_.version);

        auto repository = std::make_shared<modules::wordbank::WordBankRepository>(database_);
        auto service = std::make_shared<modules::wordbank::WordBankService>(repository);
        wordBanks_ = std::make_unique<modules::wordbank::WordBankController>(service, config_);
    }

    Server::~Server()
    {
        stop();
    }

    bool Server::isPublicPath(const std::string& path) const
    {
        return path == "/health" || path == "/ready";
    }

    void Server::installMiddleware()
    {
        http_.set_pre_routing_handler(
            [this](const httplib::Request& req, httplib::Response& res)
            {
                const std::string requestId = ch::newRequestId();
                ch::setRequestContext(requestId);
                ch::applySecurityHeaders(res, config_);

                if (req.method == "OPTIONS")
                {
                    res.status = 204;
                    return httplib::Server::HandlerResponse::Handled;
                }
                if (!isPublicPath(req.path) && !ch::isAuthorized(req, config_))
                {
                    ch::sendError(res, 401, "UNAUTHORIZED", "missing or invalid access token",
                                  requestId, config_);
                    return httplib::Server::HandlerResponse::Handled;
                }
                return httplib::Server::HandlerResponse::Unhandled;
            });

        http_.set_post_routing_handler(
            [this](const httplib::Request& req, httplib::Response& res)
            {
                ch::applySecurityHeaders(res, config_);
                core::logger::write("info", "http.request",
                                    ch::currentRequestId(),
                                    {{"method", req.method},
                                     {"path", req.path},
                                     {"status", res.status}});
            });

        http_.set_exception_handler(
            [this](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep)
            {
                std::string message = "internal server error";
                try
                {
                    if (ep) std::rethrow_exception(ep);
                }
                catch (const std::exception& error)
                {
                    message = error.what();
                }
                const std::string requestId = ch::currentRequestId();
                core::logger::write("error", "http.exception", requestId,
                                    {{"path", req.path}, {"message", message}});
                ch::sendError(res, 500, "INTERNAL_ERROR", message, requestId, config_);
            });

        http_.set_error_handler(
            [this](const httplib::Request& req, httplib::Response& res)
            {
                (void)req;
                if (!res.body.empty()) return;
                const std::string requestId = ch::currentRequestId();
                const std::string code = res.status == 404 ? "NOT_FOUND" : "INVALID_ARGUMENT";
                const std::string message = res.status == 404 ? "resource not found" : "bad request";
                ch::sendJson(res, res.status,
                             ch::errorEnvelope(code, message, requestId),
                             requestId, config_);
            });
    }

    void Server::registerRoutes()
    {
        http_.Get("/health", [this](const httplib::Request& req, httplib::Response& res)
                  { health_->health(req, res); });
        http_.Get("/ready", [this](const httplib::Request& req, httplib::Response& res)
                  { health_->ready(req, res); });

        http_.Get("/api/v1/wordbanks",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { wordBanks_->list(req, res); });
        http_.Post("/api/v1/wordbanks",
                   [this](const httplib::Request& req, httplib::Response& res)
                   { wordBanks_->create(req, res); });
        http_.Get(R"(/api/v1/wordbanks/([^/]+))",
                  [this](const httplib::Request& req, httplib::Response& res)
                  { wordBanks_->get(req, res, req.matches[1]); });
        http_.Delete(R"(/api/v1/wordbanks/([^/]+))",
                     [this](const httplib::Request& req, httplib::Response& res)
                     { wordBanks_->remove(req, res, req.matches[1]); });

        http_.Post("/api/v1/tasks",
                   [this](const httplib::Request& req, httplib::Response& res)
                   {
                       const std::string requestId = ch::currentRequestId();
                       try
                       {
                           const auto body = ch::parseJsonBody(req, requestId);
                           if (!body.contains("kind") || !body["kind"].is_string())
                           {
                               throw core::AppError(core::ErrorCode::ValidationFailed, 422,
                                                    "kind must be a string");
                           }
                           const auto task = tasks_->submit(body["kind"].get<std::string>(), body);
                           ch::sendJson(res, 202,
                                        ch::successEnvelope(task.toJson(), requestId),
                                        requestId, config_);
                       }
                       catch (const core::AppError& error)
                       {
                           ch::sendError(res, error, requestId, config_);
                       }
                   });

        http_.Get(R"(/api/v1/tasks/([^/]+))",
                  [this](const httplib::Request& req, httplib::Response& res)
                  {
                      const std::string requestId = ch::currentRequestId();
                      const auto task = tasks_->find(req.matches[1]);
                      if (!task.has_value())
                      {
                          ch::sendError(res, 404, "NOT_FOUND", "task not found", requestId, config_);
                          return;
                      }
                      ch::sendJson(res, 200,
                                   ch::successEnvelope(task->toJson(), requestId),
                                   requestId, config_);
                  });
    }

    void Server::configure()
    {
        http_.set_payload_max_length(1024 * 1024);
        http_.set_read_timeout(10);
        installMiddleware();
        registerRoutes();
    }

    std::uint16_t Server::bindAndHandshake()
    {
        port_ = static_cast<std::uint16_t>(
            config_.port == 0 ? http_.bind_to_any_port(config_.host)
                              : (http_.bind_to_port(config_.host, config_.port) ? config_.port : -1));
        if (port_ == static_cast<std::uint16_t>(-1))
        {
            throw core::AppError(core::ErrorCode::Internal, 500, "cannot bind HTTP port");
        }
        wsPort_ = hub_->start(config_.host, 0);

        core::handshake::Ready ready;
        ready.host = config_.host;
        ready.port = port_;
        ready.wsPort = wsPort_;
        ready.token = config_.token;
        ready.version = config_.version;
        core::handshake::emit(ready);

        core::logger::write("info", "server.ready", "",
                            {{"port", port_}, {"wsPort", wsPort_}, {"version", config_.version}});
        return port_;
    }

    void Server::run()
    {
        running_.store(true);
        core::logger::write("info", "server.start", "", {{"port", port_}});
        http_.listen_after_bind();
        running_.store(false);
        core::logger::write("info", "server.stopped", "", {{"port", port_}});
    }

    void Server::stop()
    {
        if (!running_.exchange(false)) return;
        tasks_->stop();
        hub_->stop();
        http_.stop();
    }
}
