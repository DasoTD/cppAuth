#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

class Logger {
public:
    static std::shared_ptr<spdlog::logger> get() {
        static auto logger = spdlog::rotating_logger_mt(
            "api_logger",      // logger name
            "logs/auth.log",   // file path
            1024 * 1024 * 5,   // 5 MB max per file
            3                  // keep 3 rotated files
        );
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        return logger;
    }
};
