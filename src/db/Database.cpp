#include "db/Database.h"
#include "core/Errors.h"
#include "core/Logger.h"

#include <stdexcept>

namespace el::db
{
    namespace
    {
        void check(int rc, sqlite3* handle, const std::string& context)
        {
            if (rc != SQLITE_OK)
            {
                const std::string message = handle != nullptr ? sqlite3_errmsg(handle) : "unknown sqlite error";
                throw core::AppError(core::ErrorCode::Internal, 500, context + ": " + message);
            }
        }
    }

    Statement::Statement(sqlite3* handle, const std::string& sql)
    {
        sqlite3_stmt* raw = nullptr;
        check(sqlite3_prepare_v2(handle, sql.c_str(), -1, &raw, nullptr), handle, "prepare failed");
        stmt_.reset(raw);
    }

    void Statement::bind(int index, const std::string& value)
    {
        check(sqlite3_bind_text(stmt_.get(), index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT),
              nullptr, "bind text failed");
    }

    void Statement::bind(int index, std::int64_t value)
    {
        check(sqlite3_bind_int64(stmt_.get(), index, value), nullptr, "bind int failed");
    }

    bool Statement::step()
    {
        const int rc = sqlite3_step(stmt_.get());
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw core::AppError(core::ErrorCode::Internal, 500,
                             std::string("step failed: ") + sqlite3_errstr(rc));
    }

    void Statement::reset()
    {
        sqlite3_reset(stmt_.get());
        sqlite3_clear_bindings(stmt_.get());
    }

    std::string Statement::text(int column) const
    {
        const auto* ptr = sqlite3_column_text(stmt_.get(), column);
        if (ptr == nullptr) return {};
        return std::string(reinterpret_cast<const char*>(ptr),
                           static_cast<std::size_t>(sqlite3_column_bytes(stmt_.get(), column)));
    }

    std::int64_t Statement::integer(int column) const
    {
        return sqlite3_column_int64(stmt_.get(), column);
    }

    bool Statement::isNull(int column) const
    {
        return sqlite3_column_type(stmt_.get(), column) == SQLITE_NULL;
    }

    Database::Database(const std::string& path)
    {
        sqlite3* raw = nullptr;
        const int rc = sqlite3_open_v2(path.c_str(), &raw,
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
                                       nullptr);
        handle_.reset(raw);
        check(rc, raw, "cannot open database");
        exec("PRAGMA journal_mode=WAL;");
        exec("PRAGMA foreign_keys=ON;");
        exec("PRAGMA busy_timeout=5000;");
    }

    void Database::exec(const std::string& sql)
    {
        char* error = nullptr;
        const int rc = sqlite3_exec(handle_.get(), sql.c_str(), nullptr, nullptr, &error);
        if (rc != SQLITE_OK)
        {
            const std::string message = error != nullptr ? error : sqlite3_errmsg(handle_.get());
            sqlite3_free(error);
            throw core::AppError(core::ErrorCode::Internal, 500, "exec failed: " + message);
        }
    }

    std::shared_ptr<Statement> Database::prepare(const std::string& sql)
    {
        return std::make_shared<Statement>(handle_.get(), sql);
    }

    void Database::migrate()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        exec(R"(
            CREATE TABLE IF NOT EXISTS schema_migrations (
                version TEXT PRIMARY KEY,
                applied_at TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS word_banks (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT NOT NULL DEFAULT '',
                word_count INTEGER NOT NULL DEFAULT 0,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_word_banks_created_at ON word_banks(created_at DESC);
            CREATE TABLE IF NOT EXISTS word_entries (
                id TEXT PRIMARY KEY,
                word_bank_id TEXT NOT NULL REFERENCES word_banks(id) ON DELETE CASCADE,
                word TEXT NOT NULL,
                translation TEXT NOT NULL DEFAULT '',
                created_at TEXT NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_word_entries_bank ON word_entries(word_bank_id);
        )");

        auto exists = prepare("SELECT 1 FROM schema_migrations WHERE version = ?");
        exists->bind(1, std::string("001_init"));
        if (!exists->step())
        {
            auto insert = prepare("INSERT INTO schema_migrations(version, applied_at) VALUES(?, ?)");
            insert->bind(1, std::string("001_init"));
            insert->bind(2, core::logger::nowIso8601Utc());
            insert->step();
        }
        ready_ = true;
    }
}
