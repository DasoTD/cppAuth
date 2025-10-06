#pragma once
#include <drogon/orm/DbClient.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

class DbLogger
{
public:
    DbLogger(drogon::orm::DbClientPtr client)
        : client_(client)
    {
        logger_ = spdlog::rotating_logger_mt("db_logger", "logs/db.log", 1024*1024*5, 3);
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        logger_->flush_on(spdlog::level::info);
    }

    template<typename... Args>
    void exec(const std::string &sql, Args&&... args)
    {
        logger_->info("SQL: {}", sql);
        client_->execSqlAsync(sql,
            [](const drogon::orm::Result &r){}, 
            [](const std::exception &e){ spdlog::error("DB Error: {}", e.what()); },
            std::forward<Args>(args)...
        );
    }

    drogon::orm::DbClientPtr getClient() { return client_; }

private:
    drogon::orm::DbClientPtr client_;
    std::shared_ptr<spdlog::logger> logger_;
};
