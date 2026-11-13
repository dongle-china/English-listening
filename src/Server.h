#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <httplib.h>

#include "config/Config.h"
#include "db/Database.h"
#include "modules/health/HealthRoutes.h"
#include "modules/tasks/TaskService.h"
#include "modules/wordbank/WordBankController.h"
#include "ws/ProgressHub.h"

namespace el
{
    class Server
    {
    public:
        Server(config::Config config, std::shared_ptr<db::Database> database);
        ~Server();

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        void configure();
        std::uint16_t bindAndHandshake();
        void run();
        void stop();
        bool running() const noexcept { return running_.load(); }

    private:
        void registerRoutes();
        void installMiddleware();
        bool isPublicPath(const std::string& path) const;

        config::Config config_;
        std::shared_ptr<db::Database> database_;
        std::shared_ptr<ws::ProgressHub> hub_;
        std::shared_ptr<modules::tasks::TaskService> tasks_;
        std::unique_ptr<modules::health::HealthRoutes> health_;
        std::unique_ptr<modules::wordbank::WordBankController> wordBanks_;
        httplib::Server http_;
        std::atomic<bool> running_{false};
        std::uint16_t port_{0};
        std::uint16_t wsPort_{0};
    };
}
