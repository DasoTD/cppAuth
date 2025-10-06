#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>

class Logger {
public:
    static std::shared_ptr<spdlog::logger> get() {
        static auto logger = spdlog::rotating_logger_mt("global_logger", "logs/global.log", 10485760, 5);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        logger->flush_on(spdlog::level::info);
        return logger;
    }
};
// Usage: Logger::get()->info("Your log message");  
