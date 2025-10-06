#pragma once
#include <drogon/orm/DbClient.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>

class DbLoggerAuto
{
public:
    explicit DbLoggerAuto(drogon::orm::DbClientPtr client, const std::string &logFile = "db_api_log.json")
        : client_(client), logfile_(logFile)
    {
        std::ofstream(logfile_, std::ios::app).close(); // ensure file exists
    }

    // Async SQL
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

    // Sync SQL
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
    std::string logfile_;
    std::mutex logMutex_;

    void logQuery(const std::string &sql,
                  const std::chrono::system_clock::time_point &start,
                  size_t rows,
                  const std::string &status,
                  const std::string &errorMsg = "")
    {
        std::lock_guard<std::mutex> lock(logMutex_);

        nlohmann::json logEntry;
        auto t = std::chrono::system_clock::to_time_t(start);
        std::stringstream ts;
        ts << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");

        logEntry["timestamp"] = ts.str();
        logEntry["sql"] = sql;
        logEntry["rows"] = rows;
        logEntry["status"] = status;
        if (!errorMsg.empty())
            logEntry["error"] = errorMsg;

        std::ofstream ofs(logfile_, std::ios::app);
        ofs << logEntry.dump() << std::endl;
    }
};
