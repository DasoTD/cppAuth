#include <drogon/drogon.h>
#include "RequestLoggerRotating.h"
#include "DbLoggerRotating.h"
#include "AuthController.h"
#include <laserpants/dotenv/dotenv.h>

int main()
{
    dotenv::init();

    drogon::orm::DbClientPtr dbClient;

    const char *driver = std::getenv("DB_DRIVER");
    if (driver && std::string(driver) == "sqlite3") {
        std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./test.db";
        dbClient = drogon::orm::DbClient::newSqlite3Client(dbFile, 1);
    } else {
        std::string connStr = "host=" + std::string(std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost") +
                              " port=" + std::string(std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432") +
                              " dbname=" + std::string(std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "cppauth") +
                              " user=" + std::string(std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres") +
                              " password=" + std::string(std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "");
        dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);
    }

    auto dbLogger = std::make_shared<DbLoggerRotating>(dbClient);

    // Register controllers with DB logger injected
    drogon::app().registerController(std::make_shared<AuthController>(dbLogger));

    // Global HTTP request logging
    drogon::app().registerFilter(std::make_shared<RequestLoggerRotating>());

    drogon::app().addListener("0.0.0.0", 8080).setThreadNum(2).run();
}






// #include <drogon/drogon.h>
// #include <drogon/orm/DbClient.h>
// #include "ApiLoggerFilter.h"
// #include "Logger.h"
// #include <laserpants/dotenv/dotenv.h>
// #include <iostream>

// using namespace drogon;

// drogon::orm::DbClientPtr dbClient;

// int main() {
//     try {
//         // Load .env
//         dotenv::init();
//         std::cout << "[INFO] .env file loaded successfully.\n";

//         // Determine DB type
//         std::string dbDriver = std::getenv("DB_DRIVER") ? std::getenv("DB_DRIVER") : "sqlite3";

//         if (dbDriver == "sqlite3") {
//             std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./test.db";
//             dbClient = drogon::orm::DbClient::newSqlite3Client(dbFile, 1);
//             std::cout << "[INFO] Using SQLite database at " << dbFile << std::endl;
//         } else if (dbDriver == "postgres") {
//             std::string dbname = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "cppauth";
//             std::string dbuser = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
//             std::string dbpass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "";
//             std::string dbhost = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
//             std::string dbport = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";

//             std::string connStr = "host=" + dbhost +
//                                   " port=" + dbport +
//                                   " dbname=" + dbname +
//                                   " user=" + dbuser +
//                                   " password=" + dbpass;

//             dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);
//             std::cout << "[INFO] Connected to PostgreSQL at " << dbhost << ":" << dbport << std::endl;
//         } else {
//             throw std::runtime_error("Unsupported DB_DRIVER: " + dbDriver);
//         }

//         // Register global API logger
//         app().registerFilter(std::make_shared<ApiLoggerFilter>());
//         app().registerFilter(std::make_shared<ApiLoggerFilter>());



//         // Start app
//         app().addListener("0.0.0.0", 8080)
//              .setThreadNum(2)
//              .run();

//     } catch (const std::exception &e) {
//         std::cerr << "[ERROR] " << e.what() << std::endl;
//         Logger::get()->error("Fatal exception: {}", e.what());
//     }

//     return 0;
// }
