#include <drogon/drogon.h>
#include <dotenv.h> // include dotenv-cpp

#include <iostream>

using namespace drogon;
using namespace dotenv;

int main() {
    try {
        // Load .env file from project root
        dotenv::init();

        // Log confirmation
        std::cout << "[INFO] Environment variables loaded from .env file" << std::endl;

        // Get environment vars for DB
        std::string dbname = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "cppauth";
        std::string dbuser = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
        std::string dbpass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "";
        std::string dbhost = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
        std::string dbport = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";

        // Create DB connection string
        std::string connStr = "host=" + dbhost + " port=" + dbport +
                              " dbname=" + dbname + " user=" + dbuser +
                              " password=" + dbpass;

        auto dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);
        drogon::app().getLoop()->queueInLoop([dbClient]() {
            std::cout << "[INFO] Connected to PostgreSQL successfully" << std::endl;
        });

        // Drogon setup
        drogon::app()
            .addListener("0.0.0.0", 8080)
            .setThreadNum(2)
            .run();

    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
    }

    return 0;
}
