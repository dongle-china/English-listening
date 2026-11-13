#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>

#include "ws/ProgressHub.h"

namespace el::modules::tasks
{
    struct TaskInfo
    {
        std::string id;
        std::string kind;
        std::string status;
        int progress{0};
        std::string createdAt;
        std::string error;
        nlohmann::json result = nlohmann::json::object();

        nlohmann::json toJson() const;
    };

    class TaskService
    {
    public:
        explicit TaskService(std::shared_ptr<ws::ProgressHub> hub);
        ~TaskService();

        TaskService(const TaskService&) = delete;
        TaskService& operator=(const TaskService&) = delete;

        TaskInfo submit(const std::string& kind, const nlohmann::json& payload);
        std::optional<TaskInfo> find(const std::string& id) const;
        void stop();

    private:
        void run(const std::string& id, nlohmann::json payload);
        void update(const std::string& id, const std::string& status, int progress,
                    const std::string& stage);

        std::shared_ptr<ws::ProgressHub> hub_;
        mutable std::mutex mutex_;
        std::vector<TaskInfo> tasks_;
        std::vector<std::thread> workers_;
        std::atomic<bool> running_{true};
        std::atomic<std::uint64_t> sequence_{0};
    };
}
