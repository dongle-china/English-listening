#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "modules/wordbank/WordBankRepository.h"

namespace el::modules::wordbank
{
    struct CreateWordBankInput
    {
        std::string name;
        std::string description;
    };

    struct ListWordBanksInput
    {
        int page{1};
        int pageSize{20};
    };

    class WordBankService
    {
    public:
        explicit WordBankService(std::shared_ptr<WordBankRepository> repository);

        WordBankRecord create(const CreateWordBankInput& input) const;
        PageResult list(const ListWordBanksInput& input) const;
        WordBankRecord getById(const std::string& id) const;
        void remove(const std::string& id) const;

    private:
        static std::string newId();
        std::shared_ptr<WordBankRepository> repository_;
    };
}
