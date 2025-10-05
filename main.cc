#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/DbTypes.h>
#include <cstdlib>
#include <iostream>
#include "AuthController.h"

// Global database client pointer
drogon::orm::DbClientPtr dbClient;

// Helper to read env variables with a fallback
std::string getEnvVar(const std::string &key, const std::string &defaultVal = "") {
    const char *val = std::getenv(key.c_str());
    return val ? std::string(val) : defaultVal;
}

int main() {
    try {
        // ---------------------- Load Environment Variables ----------------------
        std::string dbName = getEnvVar("DB_NAME", "mydb");
        std::string dbUser = getEnvVar("DB_USER", "postgres");
        std::string dbPass = getEnvVar("DB_PASS", "password");
        std::string dbHost = getEnvVar("DB_HOST", "127.0.0.1");
        std::string dbPort = getEnvVar("DB_PORT", "5432");

        std::string jwtSecret = getEnvVar("JWT_SECRET", "supersecretkey"); 
        // You can pass this secret into AuthController if you modify its constructor to accept it

        // ---------------------- Initialize Database ----------------------
        std::string dbConnStr = "dbname=" + dbName +
                                " user=" + dbUser +
                                " password=" + dbPass +
                                " host=" + dbHost +
                                " port=" + dbPort;

        dbClient = drogon::orm::DbClient::newPgClient(dbConnStr, 1 /* pool size */);

        std::cout << "[INFO] PostgreSQL database connected successfully!" << std::endl;

        // Optional test query
        dbClient->execSqlAsync(
            "SELECT NOW()",
            [](const drogon::orm::Result &r) {
                std::cout << "[INFO] DB Test Query Result: " << r[0]["now"].as<std::string>() << std::endl;
            },
            [](const std::exception &e) {
                std::cerr << "[ERROR] DB Test Query Failed: " << e.what() << std::endl;
            }
        );

        // ---------------------- Start Drogon App ----------------------
        drogon::app()
            .registerController(std::make_shared<AuthController>()) // Register controller
            .addListener("0.0.0.0", 8080)                             // Listen on all interfaces
            .run();

    } catch (const std::exception &e) {
        std::cerr << "[ERROR] Exception in main: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}



// #include <drogon/drogon.h>
// int main() {
//     //Set HTTP listener address and port
//     drogon::app().addListener("0.0.0.0", 5555);
//     //Load config file
//     //drogon::app().loadConfigFile("../config.json");
//     //drogon::app().loadConfigFile("../config.yaml");
//     //Run HTTP framework,the method will block in the internal event loop
//     drogon::app().run();
//     return 0;
// }
