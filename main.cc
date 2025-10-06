#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <laserpants/dotenv/dotenv.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "AuthController.h"
#include "BankController.h"

using namespace drogon;

drogon::orm::DbClientPtr dbClient;

int main() {
    try {
        // Load .env
        dotenv::init();
        std::cout << "[INFO] .env file loaded successfully.\n";

        // Configure logger
        auto logger = spdlog::rotating_logger_mt("bank_logger", "logs/bank.log", 1024 * 1024 * 5, 3); // 5MB, 3 files
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
        spdlog::flush_on(spdlog::level::info);

        // DB connection from env
        std::string driver = std::getenv("DB_DRIVER") ? std::getenv("DB_DRIVER") : "sqlite3";

        if(driver == "sqlite3") {
            std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./test.db";
            dbClient = drogon::orm::DbClient::newSqlite3Client(dbFile, 1);
            spdlog::info("Connected to SQLite at {}", dbFile);
        } else if(driver == "postgres") {
            std::string connStr = "host=" + std::string(std::getenv("DB_HOST")) +
                                  " port=" + std::string(std::getenv("DB_PORT")) +
                                  " dbname=" + std::string(std::getenv("DB_NAME")) +
                                  " user=" + std::string(std::getenv("DB_USER")) +
                                  " password=" + std::string(std::getenv("DB_PASS"));
            dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);
            spdlog::info("Connected to PostgreSQL at {}:{}", std::getenv("DB_HOST"), std::getenv("DB_PORT"));
        } else {
            throw std::runtime_error("Unsupported DB_DRIVER");
        }

        // JWT secret from env
        std::string jwtSecret = std::getenv("JWT_SECRET") ? std::getenv("JWT_SECRET") : "changeme";

        // Controllers
        auto authController = std::make_shared<AuthController>(dbClient, jwtSecret);
        auto bankController = std::make_shared<BankController>(dbClient, jwtSecret);

        // Register controllers with Drogon
        app().registerController(authController);
        app().registerController(bankController);

        // Run HTTP server
        app().addListener("0.0.0.0", 8080)
             .setThreadNum(4)
             .run();

    } catch (const std::exception &e) {
        spdlog::error("Fatal error: {}", e.what());
    }

    return 0;
}


// #include <drogon/drogon.h>
// #include <drogon/orm/DbClient.h>
// #include <laserpants/dotenv/dotenv.h>
// #include <iostream>
// #include "BankController.h"
// #include "RequestLoggerRotating.h"
// #include "DbLogger.h"


// using namespace drogon;

// orm::DbClientPtr dbClient;

// void initDb()
// {
//     const char* driver = std::getenv("DB_DRIVER");
//     if (!driver) throw std::runtime_error("DB_DRIVER not set in .env");

//     if (std::string(driver) == "sqlite3")
//     {
//         std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./bank.db";
//         dbClient = orm::DbClient::newSqlite3Client(dbFile, 1);
//     }
//     else if (std::string(driver) == "postgres")
//     {
//         std::string dbname = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "bank";
//         std::string dbuser = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
//         std::string dbpass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "";
//         std::string dbhost = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
//         std::string dbport = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";
//         std::string connStr = "host=" + dbhost + " port=" + dbport +
//                               " dbname=" + dbname + " user=" + dbuser + " password=" + dbpass;
//         dbClient = orm::DbClient::newPgClient(connStr, 1);
//     }
//     else
//     {
//         throw std::runtime_error("Unsupported DB_DRIVER");
//     }

//     std::cout << "[INFO] DB connected\n";
// }

// int main()
// {
//     try
//     {
//         dotenv::init();
//         std::cout << "[INFO] .env loaded\n";

//         initDb();

//         auto dbLogger = std::make_shared<DbLogger>(dbClient);

//         // Inject dbLogger->getClient() into controller:
//         BankController::setDbClient(dbLogger->getClient());

//         // Register the filter
//         app().registerFilter(std::make_shared<RequestLoggerRotating>());



//         // Add API logger
//         app().registerFilter(std::make_shared<RequestLoggerRotating>());
//         app().registerController(std::make_shared<AuthController>());
//         app().registerController(std::make_shared<BankController>());
        

//         // Inject DB client to controller
//         BankController::setDbClient(dbClient);

//         // Listen & run
//         app().addListener("0.0.0.0", 8080).setThreadNum(2).run();
//     }
//     catch (const std::exception &e)
//     {
//         std::cerr << "[ERROR] " << e.what() << std::endl;
//     }
//     return 0;
// }


// #include <drogon/drogon.h>
// #include "RequestLoggerRotating.h"
// #include "DbLoggerRotating.h"
// #include "AuthController.h"
// #include <laserpants/dotenv/dotenv.h>

// int main()
// {
//     dotenv::init();

//     drogon::orm::DbClientPtr dbClient;

//     const char *driver = std::getenv("DB_DRIVER");
//     if (driver && std::string(driver) == "sqlite3") {
//         std::string dbFile = std::getenv("DB_DATABASE") ? std::getenv("DB_DATABASE") : "./test.db";
//         dbClient = drogon::orm::DbClient::newSqlite3Client(dbFile, 1);
//     } else {
//         std::string connStr = "host=" + std::string(std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost") +
//                               " port=" + std::string(std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432") +
//                               " dbname=" + std::string(std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "cppauth") +
//                               " user=" + std::string(std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres") +
//                               " password=" + std::string(std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "");
//         dbClient = drogon::orm::DbClient::newPgClient(connStr, 1);
//     }

//     auto dbLogger = std::make_shared<DbLoggerRotating>(dbClient);
//     drogon::app().registerController(std::make_shared<BankController>(dbLogger));
//     drogon::app().registerFilter(std::make_shared<RequestLoggerRotating>());
//     drogon::app().addListener("0.0.0.0", 8080).setThreadNum(4).run();

// }






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
