#pragma once
#include <drogon/HttpFilter.h>
#include "LoggerBase.h"

class RequestLoggerRotating : public drogon::HttpFilter<RequestLoggerRotating>, public LoggerBase
{
public:
    RequestLoggerRotating() : LoggerBase("request_log") {}

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
    void logRequest(const drogon::HttpRequestPtr &req,
                    const drogon::HttpResponsePtr &resp,
                    const std::chrono::system_clock::time_point &start)
    {
        nlohmann::json entry;
        auto t = std::chrono::system_clock::to_time_t(start);
        std::stringstream ts;
        ts << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");

        entry["timestamp"] = ts.str();
        entry["method"] = req->methodString();
        entry["path"] = req->path();
        entry["query"] = req->query();
        entry["status"] = resp->statusCode();

        writeJson(entry);
    }
};
