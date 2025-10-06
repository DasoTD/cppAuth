#pragma once
#include <drogon/HttpFilter.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>

class RequestLoggerJSON : public drogon::HttpFilter<RequestLoggerJSON>
{
public:
    RequestLoggerJSON(const std::string &logFile = "request_log.json") : logfile_(logFile)
    {
        std::ofstream(logfile_, std::ios::app).close();
    }

    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb) override
    {
        auto start = std::chrono::system_clock::now();
        auto respCallback = [this, req, start, fcb](const drogon::HttpResponsePtr &resp) {
            logRequest(req, resp, start);
            fcb(resp);
        };

        nextFilter(req, std::move(respCallback));
    }

private:
    std::string logfile_;
    std::mutex logMutex_;

    void logRequest(const drogon::HttpRequestPtr &req,
                    const drogon::HttpResponsePtr &resp,
                    const std::chrono::system_clock::time_point &start)
    {
        std::lock_guard<std::mutex> lock(logMutex_);

        nlohmann::json entry;
        auto t = std::chrono::system_clock::to_time_t(start);
        std::stringstream ts;
        ts << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");

        entry["timestamp"] = ts.str();
        entry["method"] = req->methodString();
        entry["path"] = req->path();
        entry["query"] = req->query();
        entry["status"] = resp->statusCode();

        std::ofstream ofs(logfile_, std::ios::app);
        ofs << entry.dump() << std::endl;
    }
};


// #pragma once
// #include <drogon/HttpFilter.h>
// #include <spdlog/spdlog.h>
// #include <spdlog/sinks/rotating_file_sink.h>
// #include <chrono>
// #include <nlohmann/json.hpp>
// #include <memory>
// #include <vector>

// struct SqlEntry
// {
//     std::string query;
//     long long durationMs;
// };

// class RequestLoggerJSON : public drogon::HttpFilter<RequestLoggerJSON>
// {
// public:
//     RequestLoggerJSON()
//     {
//         logger_ = spdlog::rotating_logger_mt("request_logger_json", "logs/request.json", 10485760, 5);
//         logger_->set_pattern("%v");  // full JSON per line
//         logger_->flush_on(spdlog::level::info);
//     }

//     void doFilter(const drogon::HttpRequestPtr &req,
//                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
//                   drogon::FilterCallback &&fcb) override
//     {
//         auto start = std::chrono::steady_clock::now();
//         auto reqId = std::chrono::steady_clock::now().time_since_epoch().count();

//         // Attach SQL logs to request
//         req->attributes()->insert("sql_logs", std::vector<SqlEntry>{});

//         try
//         {
//             fcb([this, req, start, reqId, callback](const drogon::HttpResponsePtr &resp) {
//                 auto end = std::chrono::steady_clock::now();
//                 auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

//                 nlohmann::json j;
//                 j["req_id"] = reqId;
//                 j["method"] = req->methodString();
//                 j["path"] = req->path();
//                 j["status"] = resp->statusCode();
//                 j["duration_ms"] = duration;

//                 if (auto sqlLogsPtr = req->attributes()->get<std::vector<SqlEntry>>("sql_logs"))
//                 {
//                     j["sql_queries"] = nlohmann::json::array();
//                     for (auto &q : *sqlLogsPtr)
//                     {
//                         j["sql_queries"].push_back({{"query", q.query}, {"duration_ms", q.durationMs}});
//                     }
//                 }

//                 logger_->info(j.dump());
//                 callback(resp);
//             });
//         }
//         catch (const std::exception &e)
//         {
//             nlohmann::json j;
//             j["req_id"] = reqId;
//             j["method"] = req->methodString();
//             j["path"] = req->path();
//             j["exception"] = e.what();
//             logger_->error(j.dump());
//             throw;
//         }
//     }

// private:
//     std::shared_ptr<spdlog::logger> logger_;
// };
