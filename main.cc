#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <dotenv.h>
#include <iostream>

using namespace drogon;
using namespace dotenv;

drogon::orm::DbClientPtr dbClient;

int main() {
    try {
        // Load .env from project root
        dotenv::init();

        std::cout << "[INFO] .env file loaded successfully.\n";

        // Fetch DB connection details from environment
        std::string dbname = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "cppauth";
        std::string dbuser = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
        std::string dbpass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "";
        std::string dbhost = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
        std::string dbport = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";

        std::string connStr = "host=" + dbhost +
                              " port=" + dbport +
                              " dbname=" + dbname +
                              " user=" + dbuser +
                              " password=" + dbpass;

        // Create PostgreSQL client
        dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);

        std::cout << "[INFO] Connected to PostgreSQL at " << dbhost << ":" << dbport << std::endl;

        // Start Drogon HTTP app
        drogon::app()
            .addListener("0.0.0.0", 8080)
            .setThreadNum(2)
            .run();

    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
    }

    return 0;
}
