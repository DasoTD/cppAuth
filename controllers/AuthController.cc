#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <nlohmann/json.hpp>
#include <bcrypt/BCrypt.hpp>

using json = nlohmann::json;

extern drogon::orm::DbClientPtr dbClient; // We'll initialize in main.cpp

void AuthController::registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonReq = req->getJsonObject();
    if (!jsonReq)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    std::string username = (*jsonReq)["username"].asString();
    std::string email = (*jsonReq)["email"].asString();
    std::string password = (*jsonReq)["password"].asString();

    std::string passwordHash = BCrypt::generateHash(password);

    dbClient->execSqlAsync(
        "INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3)",
        [callback](const drogon::orm::Result &r) {
            auto resp = HttpResponse::newHttpJsonResponse({{"status", "success"}});
            callback(resp);
        },
        [callback](const std::exception &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Error creating user");
            callback(resp);
        },
        username, email, passwordHash
    );
}

void AuthController::loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonReq = req->getJsonObject();
    if (!jsonReq)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    std::string username = (*jsonReq)["username"].asString();
    std::string password = (*jsonReq)["password"].asString();

    dbClient->execSqlAsync(
        "SELECT password_hash FROM users WHERE username=$1",
        [callback, password](const drogon::orm::Result &r) {
            if (r.size() == 0)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("User not found");
                callback(resp);
                return;
            }

            std::string storedHash = r[0]["password_hash"].as<std::string>();
            if (BCrypt::validatePassword(password, storedHash))
            {
                auto resp = HttpResponse::newHttpJsonResponse({{"status", "success"}});
                callback(resp);
            }
            else
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Invalid password");
                callback(resp);
            }
        },
        [callback](const std::exception &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Error logging in");
            callback(resp);
        },
        username
    );
}
