#pragma once

// Minimal spdlog shim used for local builds. This provides only the
// functions used by the project: rotating_logger_mt and basic logging calls.

#include <memory>
#include <string>
#include <utility>
#include <iostream>

namespace spdlog {
    enum class level { trace = 0, debug, info, warn, err, critical, off };

    class logger {
    public:
        void flush_on(level) {}
        template<typename... Args>
        void info(Args&&...) { }
        template<typename... Args>
        void error(Args&&...) { }
    };

    inline std::shared_ptr<logger> rotating_logger_mt(const std::string & /*name*/, const std::string & /*filename*/, size_t /*max_size*/, size_t /*files*/) {
        return std::make_shared<logger>();
    }

    // Free functions used in code
    template<typename... Args>
    inline void info(Args&&...) { }
    template<typename... Args>
    inline void error(Args&&...) { }

    template<typename... Args>
    inline void warn(Args&&...) { }

    inline void set_default_logger(std::shared_ptr<logger>) {}
    inline void set_pattern(const std::string&) {}
    inline void flush_on(level) {}
}
