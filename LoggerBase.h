#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

class LoggerBase
{
public:
    explicit LoggerBase(const std::string &prefix) : prefix_(prefix) {}

protected:
    std::mutex logMutex_;
    std::string prefix_;

    std::string currentLogFile()
    {
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::stringstream ss;
        ss << prefix_ << "_"
           << std::put_time(std::localtime(&t), "%Y-%m-%d") << ".json";
        return ss.str();
    }

    void writeJson(const nlohmann::json &entry)
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        std::ofstream ofs(currentLogFile(), std::ios::app);
        ofs << entry.dump(4) << std::endl; // pretty print
    }
};
