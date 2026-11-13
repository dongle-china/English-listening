#pragma once

#include <memory>
#include <httplib.h>

#include "config/Config.h"
#include "modules/wordbank/WordBankService.h"

namespace el::modules::wordbank
{
    class WordBankController
    {
    public:
        WordBankController(std::shared_ptr<WordBankService> service, config::Config config);

        void list(const httplib::Request& req, httplib::Response& res) const;
        void create(const httplib::Request& req, httplib::Response& res) const;
        void get(const httplib::Request& req, httplib::Response& res, const std::string& id) const;
        void remove(const httplib::Request& req, httplib::Response& res, const std::string& id) const;

    private:
        std::shared_ptr<WordBankService> service_;
        config::Config config_;
    };
}
