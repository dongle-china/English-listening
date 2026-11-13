#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "db/Database.h"

namespace el::modules::wordbank
{
    struct WordBankRecord
    {
        std::string id;
        std::string name;
        std::string description;
        std::int64_t wordCount{0};
        std::string createdAt;
        std::string updatedAt;

        nlohmann::json toJson() const;
    };

    struct PageResult
    {
        std::vector<WordBankRecord> items;
        std::int64_t total{0};
    };

    class WordBankRepository
    {
    public:
        explicit WordBankRepository(std::shared_ptr<db::Database> database);

        PageResult list(int limit, int offset) const;
        std::optional<WordBankRecord> findById(const std::string& id) const;
        WordBankRecord create(const WordBankRecord& record) const;
        bool remove(const std::string& id) const;
        std::int64_t count() const;

    private:
        static WordBankRecord readRow(db::Statement& statement);
        std::shared_ptr<db::Database> database_;
    };
}
