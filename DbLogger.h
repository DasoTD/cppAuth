#pragma once
#include <drogon/orm/DbClient.h>
#include <chrono>
#include <drogon/HttpAppFramework.h>
#include "RequestLogger.h"

class DbLogger
{
public:
    explicit DbLogger(drogon::orm::DbClientPtr client)
        : client_(client) {}

    void execSqlAsync(const std::string &sql,
                      const drogon::orm::ResultCallback &rcb,
                      const drogon::orm::ExceptionCallback &ecb = nullptr)
    {
        auto start = std::chrono::steady_clock::now();
        auto req = drogon::app().getCurrentHttpRequest();

        auto wrappedRcb = [req, start, sql, rcb](const drogon::orm::Result &r) {
            auto end = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (req)
            {
                auto &logs = req->attributes()->get<std::vector<SqlEntry>>("sql_logs");
                logs.push_back({sql, dur});
            }
            rcb(r);
        };

        auto wrappedEcb = [req, start, sql, ecb](const drogon::orm::DrogonDbException &ex) {
            auto end = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (req)
            {
                auto &logs = req->attributes()->get<std::vector<SqlEntry>>("sql_logs");
                logs.push_back({sql + " [FAILED: " + ex.base().what() + "]", dur});
            }
            if (ecb)
                ecb(ex);
        };

        client_->execSqlAsync(sql, wrappedRcb, wrappedEcb);
    }

    drogon::orm::Result execSqlSync(const std::string &sql)
    {
        auto start = std::chrono::steady_clock::now();
        auto req = drogon::app().getCurrentHttpRequest();
        drogon::orm::Result res;
        try
        {
            res = client_->execSqlSync(sql);
            auto end = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (req)
            {
                auto &logs = req->attributes()->get<std::vector<SqlEntry>>("sql_logs");
                logs.push_back({sql, dur});
            }
        }
        catch (const drogon::orm::DrogonDbException &ex)
        {
            auto end = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            if (req)
            {
                auto &logs = req->attributes()->get<std::vector<SqlEntry>>("sql_logs");
                logs.push_back({sql + " [FAILED: " + ex.base().what() + "]", dur});
            }
            throw;
        }
        return res;
    }

    drogon::orm::DbClientPtr client() const { return client_; }

private:
    drogon::orm::DbClientPtr client_;
};


// #pragma once
// #include <drogon/orm/DbClient.h>
// #include <spdlog/spdlog.h>
// #include <memory>
// #include <string>
// #include <chrono>

// class DbLogger
// {
// public:
//     explicit DbLogger(drogon::orm::DbClientPtr client)
//         : client_(client), logger_(spdlog::rotating_logger_mt("db_logger", "logs/db.log", 10485760, 5))
//     {
//         logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
//         logger_->flush_on(spdlog::level::info);
//     }

//     // Synchronous SQL execution
//     drogon::orm::Result execSqlSync(const std::string &sql)
//     {
//         auto start = std::chrono::steady_clock::now();
//         auto res = client_->execSqlSync(sql);
//         auto end = std::chrono::steady_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//         logger_->info("SQL Sync: '{}' -> {} rows ({} ms)", sql, res.size(), duration);
//         return res;
//     }

//     // Asynchronous SQL execution
//     void execSqlAsync(const std::string &sql, drogon::orm::ResultCallback &&rcb, drogon::orm::ExceptionCallback &&ecb = nullptr)
//     {
//         auto start = std::chrono::steady_clock::now();
//         client_->execSqlAsync(
//             sql,
//             [this, start, sql, rcb = std::move(rcb)](const drogon::orm::Result &res) mutable {
//                 auto end = std::chrono::steady_clock::now();
//                 auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//                 logger_->info("SQL Async: '{}' -> {} rows ({} ms)", sql, res.size(), duration);
//                 rcb(res);
//             },
//             [this, sql, ecb = std::move(ecb)](const std::exception &ex) mutable {
//                 logger_->error("SQL Async ERROR: '{}' -> {}", sql, ex.what());
//                 if (ecb)
//                     ecb(ex);
//             });
//     }

//     // Forward other DbClient methods as needed
//     drogon::orm::DbClientPtr client() const { return client_; }

// private:
//     drogon::orm::DbClientPtr client_;
//     std::shared_ptr<spdlog::logger> logger_;
// };
