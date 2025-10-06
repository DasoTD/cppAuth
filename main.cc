#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>
#include <laserpants/dotenv/dotenv.h>
#include <iostream>
#include <trantor/utils/Logger.h>
#include "ApiLoggerFilter.h"


using namespace drogon;


drogon::orm::DbClientPtr dbClient;

int main() {
    try {
        // Load .env from project root
        dotenv::init();

        std::cout << "[INFO] .env file loaded successfully.\n";
        system("mkdir -p logs"); 

    //     // Set the log path for Trantor
    //    auto asyncLogger = std::make_shared<trantor::AsyncFileLogger>("logs/cppAuth.log", 1024 * 1024);
    //     asyncLogger->start();
        
        
        // Fetch DB connection details from environment
        std::string dbname = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "cppauth";
        std::string dbuser = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
        std::string dbpass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "";
        std::string dbhost = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
        std::string dbport = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";

        std::string dbDriver = std::getenv("DB_DRIVER") ? std::getenv("DB_DRIVER") : "postgresql";

        if (dbDriver == "sqlite3") {
            std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./test.db";
            dbClient = drogon::orm::DbClient::newSqlite3Client(dbFile, 1);
            std::cout << "[INFO] Connected to SQLite at " << dbFile << std::endl;
        } else if (dbDriver == "postgresql") {
            std::string connStr = "host=" + dbhost +
                                " port=" + dbport +
                                " dbname=" + dbname +
                                " user=" + dbuser +
                                " password=" + dbpass;
            dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);
            std::cout << "[INFO] Connected to PostgreSQL at " << dbhost << ":" << dbport << std::endl;
        } else {
            throw std::runtime_error("Unsupported DB_DRIVER: " + dbDriver);
        }

        // std::string connStr = "host=" + dbhost +
        //                       " port=" + dbport +
        //                       " dbname=" + dbname +
        //                       " user=" + dbuser +
        //                       " password=" + dbpass;

        // std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./test.db";
    
        // // Create PostgreSQL client
        // dbClient = drogon::orm::DbClient::newSqlite3Client(dbFile, 1);

        // std::cout << "[INFO] Connected to PostgreSQL at " << dbhost << ":" << dbport << std::endl;

        // Start Drogon HTTP app
        drogon::app().registerFilter(std::make_shared<ApiLoggerFilter>());
        drogon::app()
            .addListener("0.0.0.0", 8087)
            .setThreadNum(2)
            .run();

    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
    }

    return 0;
}
