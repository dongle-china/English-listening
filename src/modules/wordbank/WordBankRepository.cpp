#include "modules/wordbank/WordBankRepository.h"

#include <mutex>
#include <utility>

namespace el::modules::wordbank
{
    nlohmann::json WordBankRecord::toJson() const
    {
        return {
            {"id", id},
            {"name", name},
            {"description", description},
            {"wordCount", wordCount},
            {"createdAt", createdAt},
            {"updatedAt", updatedAt}
        };
    }

    WordBankRepository::WordBankRepository(std::shared_ptr<db::Database> database)
        : database_(std::move(database))
    {
    }

    WordBankRecord WordBankRepository::readRow(db::Statement& statement)
    {
        WordBankRecord record;
        record.id = statement.text(0);
        record.name = statement.text(1);
        record.description = statement.text(2);
        record.wordCount = statement.integer(3);
        record.createdAt = statement.text(4);
        record.updatedAt = statement.text(5);
        return record;
    }

    PageResult WordBankRepository::list(int limit, int offset) const
    {
        std::lock_guard<std::mutex> guard(database_->mutex());
        PageResult result;
        result.total = count();

        auto statement = database_->prepare(
            "SELECT id, name, description, word_count, created_at, updated_at "
            "FROM word_banks ORDER BY created_at DESC, id DESC LIMIT ? OFFSET ?");
        statement->bind(1, static_cast<std::int64_t>(limit));
        statement->bind(2, static_cast<std::int64_t>(offset));
        while (statement->step())
        {
            result.items.push_back(readRow(*statement));
        }
        return result;
    }

    std::int64_t WordBankRepository::count() const
    {
        auto statement = database_->prepare("SELECT COUNT(*) FROM word_banks");
        if (!statement->step()) return 0;
        return statement->integer(0);
    }

    std::optional<WordBankRecord> WordBankRepository::findById(const std::string& id) const
    {
        std::lock_guard<std::mutex> guard(database_->mutex());
        auto statement = database_->prepare(
            "SELECT id, name, description, word_count, created_at, updated_at "
            "FROM word_banks WHERE id = ?");
        statement->bind(1, id);
        if (!statement->step()) return std::nullopt;
        return readRow(*statement);
    }

    WordBankRecord WordBankRepository::create(const WordBankRecord& record) const
    {
        std::lock_guard<std::mutex> guard(database_->mutex());
        auto statement = database_->prepare(
            "INSERT INTO word_banks(id, name, description, word_count, created_at, updated_at) "
            "VALUES(?, ?, ?, ?, ?, ?)");
        statement->bind(1, record.id);
        statement->bind(2, record.name);
        statement->bind(3, record.description);
        statement->bind(4, record.wordCount);
        statement->bind(5, record.createdAt);
        statement->bind(6, record.updatedAt);
        statement->step();
        return record;
    }

    bool WordBankRepository::remove(const std::string& id) const
    {
        std::lock_guard<std::mutex> guard(database_->mutex());
        auto statement = database_->prepare("DELETE FROM word_banks WHERE id = ?");
        statement->bind(1, id);
        statement->step();
        return true;
    }
}
