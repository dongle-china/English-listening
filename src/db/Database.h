#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <sqlite3.h>

namespace el::db
{
    class Statement
    {
    public:
        Statement(sqlite3* handle, const std::string& sql);
        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;
        ~Statement() = default;

        void bind(int index, const std::string& value);
        void bind(int index, std::int64_t value);
        bool step();
        void reset();
        std::string text(int column) const;
        std::int64_t integer(int column) const;
        bool isNull(int column) const;

    private:
        struct Deleter
        {
            void operator()(sqlite3_stmt* ptr) const noexcept
            {
                if (ptr != nullptr) sqlite3_finalize(ptr);
            }
        };

        std::unique_ptr<sqlite3_stmt, Deleter> stmt_;
    };

    class Database
    {
    public:
        explicit Database(const std::string& path);
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;
        ~Database() = default;

        void exec(const std::string& sql);
        void migrate();
        std::shared_ptr<Statement> prepare(const std::string& sql);
        sqlite3* handle() const noexcept { return handle_.get(); }
        std::mutex& mutex() noexcept { return mutex_; }
        bool isReady() const noexcept { return ready_; }

    private:
        struct Deleter
        {
            void operator()(sqlite3* ptr) const noexcept
            {
                if (ptr != nullptr) sqlite3_close_v2(ptr);
            }
        };

        std::unique_ptr<sqlite3, Deleter> handle_;
        std::mutex mutex_;
        bool ready_{false};
    };
}
