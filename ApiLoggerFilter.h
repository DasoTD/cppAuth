#pragma once
#include <drogon/HttpFilter.h>
#include "Logger.h"

using namespace drogon;

class ApiLoggerFilter : public HttpFilter<ApiLoggerFilter> {
public:
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb) override {
        auto logger = Logger::get();

        // Log incoming request
        logger->info("Request: {} {}", req->methodName(), req->path());

        // Continue processing request
        auto callback = [logger](const HttpResponsePtr &resp) {
            logger->info("Response: {} {}", resp->statusCode(), resp->content());
        };

        fcb([callback](const HttpResponsePtr &resp){
            callback(resp);
        });
    }

    const std::set<std::string> &pathFilters() const override {
        static std::set<std::string> paths{}; // empty = all paths
        return paths;
    }

    const std::set<HttpMethod> &methodFilters() const override {
        static std::set<HttpMethod> methods{}; // empty = all methods
        return methods;
    }
};
