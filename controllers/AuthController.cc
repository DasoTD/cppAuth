#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <nlohmann/json.hpp>
#include <bcrypt/BCrypt.hpp>
#include <chrono>
#include <ctime>


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
                auto accessToken = generateAccessToken(username);
                auto refreshToken = generateRefreshToken(username);

                auto respJson = json{
                    {"status", "success"},
                    {"access_token", accessToken},
                    {"refresh_token", refreshToken}
                };
                callback(HttpResponse::newHttpJsonResponse(respJson));
            }

            // if (BCrypt::validatePassword(password, storedHash))
            // {
            //     auto resp = HttpResponse::newHttpJsonResponse({{"status", "success"}});
            //     callback(resp);
            // }
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

std::string AuthController::generateAccessToken(const std::string &username) {
    using namespace std::chrono;
    auto token = jwt::create()
        .set_issuer("my_drogon_app")
        .set_subject(username)
        .set_issued_at(system_clock::to_time_t(system_clock::now()))
        .set_expires_at(system_clock::to_time_t(system_clock::now() + minutes{15}))
        .sign(jwt::algorithm::hs256{jwtSecret});
    return token;
}

std::string AuthController::generateRefreshToken(const std::string &username) {
    using namespace std::chrono;
    auto token = jwt::create()
        .set_issuer("my_drogon_app")
        .set_subject(username)
        .set_issued_at(system_clock::to_time_t(system_clock::now()))
        .set_expires_at(system_clock::to_time_t(system_clock::now() + hours{24*7})) // 7 days
        .sign(jwt::algorithm::hs256{jwtSecret});

    std::lock_guard<std::mutex> lock(refreshMutex);
    refreshTokens[username] = token;
    return token;
}


void AuthController::refreshToken(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto jsonReq = req->getJsonObject();
    if (!jsonReq) {
        callback(HttpResponse::newHttpResponse(k400BadRequest, "Invalid JSON"));
        return;
    }

    std::string username = (*jsonReq)["username"].asString();
    std::string oldToken = (*jsonReq)["refresh_token"].asString();

    std::lock_guard<std::mutex> lock(refreshMutex);
    if (refreshTokens.find(username) == refreshTokens.end() || refreshTokens[username] != oldToken) {
        callback(HttpResponse::newHttpResponse(k401Unauthorized, "Invalid refresh token"));
        return;
    }

    // Verify JWT
    try {
        auto decoded = jwt::decode(oldToken);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwtSecret})
            .with_issuer("my_drogon_app");
        verifier.verify(decoded);

        auto newAccess = generateAccessToken(username);
        auto newRefresh = generateRefreshToken(username); // rotates the token

        auto respJson = json{
            {"access_token", newAccess},
            {"refresh_token", newRefresh}
        };
        callback(HttpResponse::newHttpJsonResponse(respJson));
    } catch (const std::exception &e) {
        callback(HttpResponse::newHttpResponse(k401Unauthorized, "Refresh token expired or invalid"));
    }
}

void AuthController::getProfile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    std::string username = req->getAttribute<std::string>("username"); // set by middleware

    dbClient->execSqlAsync(
        "SELECT id, username, email, created_at FROM users WHERE username=$1",
        [callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(HttpResponse::newHttpResponse(k404NotFound, "User not found"));
                return;
            }
            json respJson = {
                {"id", r[0]["id"].as<int>()},
                {"username", r[0]["username"].as<std::string>()},
                {"email", r[0]["email"].as<std::string>()},
                {"created_at", r[0]["created_at"].as<std::string>()}
            };
            callback(HttpResponse::newHttpJsonResponse(respJson));
        },
        [callback](const std::exception &e) {
            callback(HttpResponse::newHttpResponse(k500InternalServerError, "Database error"));
        },
        username
    );
}

