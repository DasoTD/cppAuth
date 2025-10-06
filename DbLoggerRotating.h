#pragma once
#include <drogon/orm/DbClient.h>
#include "LoggerBase.h"

class DbLoggerRotating : public LoggerBase
{
public:
    explicit DbLoggerRotating(drogon::orm::DbClientPtr client)
        : LoggerBase("db_log"), client_(client) {}

    void execSqlAsync(const std::string &sql,
                      const drogon::orm::ResultCallback &rcb,
                      const drogon::orm::ExceptionCallback &ecb = nullptr)
    {
        auto start = std::chrono::system_clock::now();

        client_->execSqlAsync(sql,
            [this, sql, start, rcb](const drogon::orm::Result &r) {
                logQuery(sql, start, r.size(), "success");
                if (rcb) rcb(r);
            },
            [this, sql, start, ecb](const drogon::orm::DrogonDbException &ex) {
                logQuery(sql, start, 0, "error", ex.base().what());
                if (ecb) ecb(ex);
            });
    }

    drogon::orm::Result execSqlSync(const std::string &sql)
    {
        auto start = std::chrono::system_clock::now();
        try
        {
            auto result = client_->execSqlSync(sql);
            logQuery(sql, start, result.size(), "success");
            return result;
        }
        catch (const drogon::orm::DrogonDbException &ex)
        {
            logQuery(sql, start, 0, "error", ex.base().what());
            throw;
        }
    }

private:
    drogon::orm::DbClientPtr client_;

    void logQuery(const std::string &sql,
                  const std::chrono::system_clock::time_point &start,
                  size_t rows,
                  const std::string &status,
                  const std::string &errorMsg = "")
    {
        nlohmann::json entry;
        auto t = std::chrono::system_clock::to_time_t(start);
        std::stringstream ts;
        ts << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");

        entry["timestamp"] = ts.str();
        entry["sql"] = sql;
        entry["rows"] = rows;
        entry["status"] = status;
        if (!errorMsg.empty())
            entry["error"] = errorMsg;

        writeJson(entry);
    }
};
