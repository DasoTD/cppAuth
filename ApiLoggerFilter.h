#pragma once
#include <drogon/HttpFilter.h>
#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>

class ApiLoggerFilter : public drogon::HttpFilter<ApiLoggerFilter>
{
public:
    ApiLoggerFilter() {
        // Setup rotating file logger (10MB per file, 5 files)
        try {
            logger_ = spdlog::rotating_logger_mt("api_logger", "logs/api.log", 10485760, 5);
            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            logger_->flush_on(spdlog::level::info);
        } catch (const spdlog::spdlog_ex &ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
    }

    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb) override
    {
        auto start = std::chrono::steady_clock::now();

        auto method = req->methodString();
        auto path = req->path();
        auto query = req->getParameters();

        // Log incoming request
        logger_->info("Incoming request: {} {}", method, path);

        // Call the next handler
        fcb([this, req, start](drogon::ReqResult result, const drogon::HttpResponsePtr &resp) {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            int status = resp ? resp->statusCode() : 0;
            logger_->info("Request finished: {} {} -> {} ({} ms)", req->methodString(), req->path(), status, duration);

            // Optional: Log DB queries for this request if you have your dbClient wrapper
            if (dbClient) {
                logger_->info("DB Client connected: {}", dbClient->clientName());
            }
        });
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};
