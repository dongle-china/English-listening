#include "modules/tasks/TaskService.h"
#include "core/Errors.h"
#include "core/Logger.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utility>

namespace el::modules::tasks
{
    nlohmann::json TaskInfo::toJson() const
    {
        nlohmann::json body = {
            {"id", id},
            {"kind", kind},
            {"status", status},
            {"progress", progress},
            {"createdAt", createdAt},
            {"result", result}
        };
        if (!error.empty()) body["error"] = error;
        return body;
    }

    TaskService::TaskService(std::shared_ptr<ws::ProgressHub> hub) : hub_(std::move(hub))
    {
    }

    TaskService::~TaskService()
    {
        stop();
    }

    TaskInfo TaskService::submit(const std::string& kind, const nlohmann::json& payload)
    {
        if (kind.empty())
        {
            throw core::AppError(core::ErrorCode::ValidationFailed, 422, "kind must not be empty");
        }

        const std::uint64_t sequence = sequence_.fetch_add(1);
        std::ostringstream oss;
        oss << "task-" << std::hex << std::setw(16) << std::setfill('0') << sequence;

        TaskInfo task;
        task.id = oss.str();
        task.kind = kind;
        task.status = "queued";
        task.createdAt = core::logger::nowIso8601Utc();

        {
            std::lock_guard<std::mutex> guard(mutex_);
            tasks_.push_back(task);
        }

        workers_.emplace_back([this, id = task.id, payload] { run(id, payload); });
        core::logger::write("info", "task.submitted", "", {{"id", task.id}, {"kind", kind}});
        return task;
    }

    std::optional<TaskInfo> TaskService::find(const std::string& id) const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        const auto it = std::find_if(tasks_.begin(), tasks_.end(),
                                     [&id](const TaskInfo& task) { return task.id == id; });
        if (it == tasks_.end()) return std::nullopt;
        return *it;
    }

    void TaskService::update(const std::string& id, const std::string& status, int progress,
                             const std::string& stage)
    {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            for (auto& task : tasks_)
            {
                if (task.id == id)
                {
                    task.status = status;
                    task.progress = progress;
                    break;
                }
            }
        }
        if (hub_)
        {
            hub_->broadcast({
                {"event", "task.progress"},
                {"data", {{"id", id}, {"status", status}, {"progress", progress}, {"stage", stage}}},
                {"ts", core::logger::nowIso8601Utc()}
            });
        }
    }

    void TaskService::run(const std::string& id, nlohmann::json payload)
    {
        (void)payload;
        try
        {
            update(id, "running", 0, "started");
            for (int step = 1; step <= 10 && running_.load(); ++step)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(120));
                update(id, "running", step * 10, "processing");
            }
            if (!running_.load()) return;
            update(id, "completed", 100, "finished");
            core::logger::write("info", "task.completed", "", {{"id", id}});
        }
        catch (const std::exception& error)
        {
            std::lock_guard<std::mutex> guard(mutex_);
            for (auto& task : tasks_)
            {
                if (task.id == id)
                {
                    task.status = "failed";
                    task.error = error.what();
                    break;
                }
            }
            core::logger::write("error", "task.failed", "", {{"id", id}, {"message", error.what()}});
        }
    }

    void TaskService::stop()
    {
        running_.store(false);
        for (auto& worker : workers_)
        {
            if (worker.joinable()) worker.join();
        }
        workers_.clear();
    }
}
