#pragma once
#include <drogon/HttpFilter.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <chrono>
#include <sstream>
#include <memory>
#include <vector>
#include <unordered_map>

struct SqlEntry
{
    std::string query;
    long long durationMs;
};

class RequestLoggerFilter : public drogon::HttpFilter<RequestLoggerFilter>
{
public:
    RequestLoggerFilter()
    {
        logger_ = spdlog::rotating_logger_mt("request_logger", "logs/request.log", 10485760, 5);
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        logger_->flush_on(spdlog::level::info);
    }

    void doFilter(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  drogon::FilterCallback &&fcb) override
    {
        auto start = std::chrono::steady_clock::now();
        auto reqId = std::chrono::steady_clock::now().time_since_epoch().count();

        // Attach empty SQL log vector to request attributes
        req->attributes()->insert("sql_logs", std::vector<SqlEntry>{});

        try
        {
            fcb([this, req, start, reqId, callback](const drogon::HttpResponsePtr &resp) {
                auto end = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                auto sqlLogsPtr = req->attributes()->get<std::vector<SqlEntry>>("sql_logs");
                std::ostringstream oss;
                oss << "REQ_ID=" << reqId
                    << " " << req->methodString() << " " << req->path()
                    << " STATUS=" << resp->statusCode()
                    << " DURATION=" << duration << "ms";

                if (sqlLogsPtr && !sqlLogsPtr->empty())
                {
                    oss << " QUERIES=[";
                    for (size_t i = 0; i < sqlLogsPtr->size(); ++i)
                    {
                        oss << "(" << sqlLogsPtr->at(i).durationMs << "ms) " << sqlLogsPtr->at(i).query;
                        if (i + 1 < sqlLogsPtr->size())
                            oss << ", ";
                    }
                    oss << "]";
                }

                logger_->info(oss.str());
                callback(resp);
            });
        }
        catch (const std::exception &e)
        {
            logger_->error("REQ_ID={} Exception: {}", reqId, e.what());
            throw;
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};
