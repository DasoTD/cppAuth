#include <drogon/drogon.h>
#include "controllers/AuthController.h"
#include "JwtMiddleware.h"

using namespace drogon;

orm::DbClientPtr dbClient;

int main() {
    dbClient = drogon::orm::DbClient::newPgClient(
        "dbname=yourdb user=youruser password=yourpass host=127.0.0.1 port=5432", 4
    );

    auto authController = std::make_shared<AuthController>();
    drogon::app().registerController(authController);

    // Register JWT middleware for all routes under /api
    drogon::app().registerFilter(std::make_shared<JwtMiddleware>(), "/api/*");

    drogon::app().run();
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
