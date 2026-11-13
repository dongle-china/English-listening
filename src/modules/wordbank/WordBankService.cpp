#include "modules/wordbank/WordBankService.h"
#include "core/Errors.h"
#include "core/Logger.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>
#include <utility>

namespace el::modules::wordbank
{
    namespace
    {
        std::string trim(const std::string& value)
        {
            const auto first = std::find_if_not(value.begin(), value.end(),
                                                [](unsigned char ch) { return std::isspace(ch) != 0; });
            const auto last = std::find_if_not(value.rbegin(), value.rend(),
                                               [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
            if (first >= last) return {};
            return std::string(first, last);
        }
    }

    WordBankService::WordBankService(std::shared_ptr<WordBankRepository> repository)
        : repository_(std::move(repository))
    {
    }

    std::string WordBankService::newId()
    {
        static thread_local std::mt19937_64 gen(std::random_device{}());
        std::uniform_int_distribution<std::uint64_t> dist;
        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(16) << dist(gen) << std::setw(16) << dist(gen);
        return oss.str();
    }

    WordBankRecord WordBankService::create(const CreateWordBankInput& input) const
    {
        const std::string name = trim(input.name);
        if (name.empty())
        {
            throw core::AppError(core::ErrorCode::ValidationFailed, 422, "name must not be empty");
        }
        if (name.size() > 128)
        {
            throw core::AppError(core::ErrorCode::ValidationFailed, 422, "name must be at most 128 characters");
        }
        if (input.description.size() > 1024)
        {
            throw core::AppError(core::ErrorCode::ValidationFailed, 422, "description is too long");
        }

        const std::string now = core::logger::nowIso8601Utc();
        WordBankRecord record;
        record.id = newId();
        record.name = name;
        record.description = trim(input.description);
        record.createdAt = now;
        record.updatedAt = now;
        return repository_->create(record);
    }

    PageResult WordBankService::list(const ListWordBanksInput& input) const
    {
        const int page = std::max(1, input.page);
        const int pageSize = std::clamp(input.pageSize, 1, 100);
        const int offset = (page - 1) * pageSize;
        return repository_->list(pageSize, offset);
    }

    WordBankRecord WordBankService::getById(const std::string& id) const
    {
        auto record = repository_->findById(id);
        if (!record.has_value())
        {
            throw core::AppError(core::ErrorCode::NotFound, 404, "word bank not found");
        }
        return *record;
    }

    void WordBankService::remove(const std::string& id) const
    {
        getById(id);
        repository_->remove(id);
    }
}
