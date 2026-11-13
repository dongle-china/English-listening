#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <asio.hpp>
#include <nlohmann/json.hpp>

namespace el::ws
{
    class ProgressHub
    {
    public:
        explicit ProgressHub(std::string token);
        ~ProgressHub();

        ProgressHub(const ProgressHub&) = delete;
        ProgressHub& operator=(const ProgressHub&) = delete;

        std::uint16_t start(const std::string& host, std::uint16_t port);
        void stop();
        void broadcast(const nlohmann::json& event);

    private:
        class Session;

        void acceptLoop();
        void handle(std::shared_ptr<asio::ip::tcp::socket> socket);

        std::string token_;
        asio::io_context io_;
        std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
        std::thread thread_;
        std::atomic<bool> running_{false};
        std::mutex mutex_;
        std::vector<std::weak_ptr<Session>> sessions_;
        std::vector<std::thread> workers_;
        std::uint16_t port_{0};
    };
}
