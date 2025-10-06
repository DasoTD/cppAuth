#pragma once
#include <drogon/HttpFilter.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <chrono>

class RequestLoggerRotating : public drogon::HttpFilter<RequestLoggerRotating>
{
public:
    RequestLoggerRotating()
    {
        logger_ = spdlog::rotating_logger_mt("api_logger", "logs/api.log", 1024*1024*5, 3);
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        logger_->flush_on(spdlog::level::info);
    }

    void doFilter(const drogon::HttpRequestPtr &req,
                  std::function<void (const drogon::HttpResponsePtr &)> &&f,
                  FilterCallback &&fc) override
    {
        auto start = std::chrono::steady_clock::now();
        auto path = req->path();
        auto method = req->methodString();

        fc([start, path, method, f, this](const drogon::HttpResponsePtr &resp) {
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            logger_->info("{} {} => {} ({}ms)", method, path, static_cast<int>(resp->getStatusCode()), ms);

            f(resp); // continue the chain
        });
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};
