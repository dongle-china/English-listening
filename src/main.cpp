#include "Server.h"
#include "config/Config.h"
#include "core/Logger.h"
#include "db/Database.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <nlohmann/json.hpp>

namespace
{
    std::atomic<bool> g_stopRequested{false};

    void onSignal(int)
    {
        g_stopRequested.store(true);
    }
}

int main()
{
    try
    {
        const auto config = el::config::load();
        el::core::logger::init(config.logLevel);

        auto database = std::make_shared<el::db::Database>(config.dbPath);
        database->migrate();

        el::Server server(config, database);
        server.configure();
        server.bindAndHandshake();

        std::signal(SIGINT, onSignal);
        std::signal(SIGTERM, onSignal);

        std::thread worker([&server] { server.run(); });
        while (!g_stopRequested.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        server.stop();
        if (worker.joinable()) worker.join();
        return 0;
    }
    catch (const std::exception& error)
    {
        const nlohmann::json payload = {
            {"event", "fatal"},
            {"error", {{"code", "STARTUP_FAILED"}, {"message", error.what()}}},
            {"ts", el::core::logger::nowIso8601Utc()}
        };
        std::cerr << payload.dump() << std::endl;
        return 1;
    }
}
