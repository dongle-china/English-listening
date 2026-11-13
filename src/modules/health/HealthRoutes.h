#pragma once

#include <memory>
#include <string>
#include <httplib.h>

#include "config/Config.h"
#include "db/Database.h"

namespace el::modules::health
{
    class HealthRoutes
    {
    public:
        HealthRoutes(std::shared_ptr<db::Database> database, config::Config config, std::string version);

        void health(const httplib::Request& req, httplib::Response& res) const;
        void ready(const httplib::Request& req, httplib::Response& res) const;

    private:
        std::shared_ptr<db::Database> database_;
        config::Config config_;
        std::string version_;
    };
}
