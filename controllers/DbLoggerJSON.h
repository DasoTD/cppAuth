#pragma once
#include <drogon/orm/DbClient.h>
#include <chrono>
#include <drogon/HttpAppFramework.h>
#include "RequestLoggerJSON.h"

class DbLoggerJSON
{
public:
    explicit DbLoggerJSON(drogon::orm::DbClientPtr client)
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
