// #include "AuthController.h"
// #include <nlohmann/json.hpp>

// void AuthController::registerUser(const HttpRequestPtr &req,
//                                   std::function<void(const HttpResponsePtr &)> &&callback)
// {
//     auto json = req->getJsonObject();
//     if (!json || !(*json)["username"].is_string() || !(*json)["password"].is_string())
//     {
//         auto resp = HttpResponse::newHttpResponse();
//         resp->setStatusCode(k400BadRequest);
//         resp->setBody("Invalid input");
//         callback(resp);
//         return;
//     }

//     std::string username = (*json)["username"].asString();
//     std::string password = (*json)["password"].asString();

//     std::string sql = "INSERT INTO users(username, password) VALUES('" + username + "', '" + password + "')";

//     dbService_->execSqlAsync(sql,
//         [callback](const drogon::orm::Result &r) {
//             auto resp = HttpResponse::newHttpJsonResponse({{"status", "success"}});
//             callback(resp);
//         },
//         [callback](const drogon::orm::DrogonDbException &ex) {
//             auto resp = HttpResponse::newHttpJsonResponse({{"status", "error"}, {"msg", ex.base().what()}});
//             resp->setStatusCode(k500InternalServerError);
//             callback(resp);
//         });
// }


#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <drogon/orm/DbTypes.h>
#include <nlohmann/json.hpp>
#include <laserpants/dotenv/dotenv.h>
#include <chrono>
#include <drogon/drogon.h>
#include <string>
#include <functional>
#include "AuthController.h"
#include "AuthController.h"
#include <cstdlib>
#include <iostream>

using json = nlohmann::json;

extern drogon::orm::DbClientPtr dbClient; // Initialized in main.cpp

// ---------------------- Helper Functions ----------------------

namespace {
    // Returns nullptr if JSON is invalid
    std::shared_ptr<Json::Value> validateJson(const HttpRequestPtr &req) {
        auto jsonObj = req->getJsonObject();
        if (!jsonObj) return nullptr;
        return jsonObj;
    }

    HttpResponsePtr errorResponse(const std::string &msg, HttpStatusCode code) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(code);
        resp->setBody(msg);
        return resp;
    }
}

std::string hashPassword(const std::string &password) {
    return bcrypt::generateHash(password);
}
bool verifyPassword(const std::string &password, const std::string &hash) {
    return bcrypt::validatePassword(password, hash);
}



AuthController::AuthController() {
    const char *envSecret = std::getenv("JWT_SECRET");
    if (envSecret && std::string(envSecret).size() > 0) {
        jwtSecret = envSecret;
        std::cout << "[INFO] JWT_SECRET loaded from .env file." << std::endl;
    } else {
        jwtSecret = "default_secret_key";
        std::cerr << "[WARN] JWT_SECRET not set. Using fallback secret!" << std::endl;
    }
}



// ---------------------- Register User ----------------------
void AuthController::registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonReq = validateJson(req);
    if (!jsonReq) {
        callback(errorResponse("Invalid JSON", k400BadRequest));
        return;
    }

    std::string username = (*jsonReq)["username"].asString();
    std::string email = (*jsonReq)["email"].asString();
    std::string password = (*jsonReq)["password"].asString();

    std::string passwordHash = hashPassword(password);

    dbClient->execSqlAsync(
        "INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3)",
        [callback](const drogon::orm::Result &) {
            Json::Value resp;
            resp["status"] = "success";
            callback(HttpResponse::newHttpJsonResponse(resp));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            callback(errorResponse("Error creating user", k500InternalServerError));
        },
        username, email, passwordHash
    );
}

// ---------------------- Login User ----------------------
void AuthController::loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonReq = validateJson(req);
    if (!jsonReq) {
        callback(errorResponse("Invalid JSON", k400BadRequest));
        return;
    }

    std::string username = (*jsonReq)["username"].asString();
    std::string password = (*jsonReq)["password"].asString();

    dbClient->execSqlAsync(
        "SELECT password_hash FROM users WHERE username=$1",
        [this, callback, password, username](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(errorResponse("User not found", k401Unauthorized));
                return;
            }

            std::string storedHash = r[0]["password_hash"].as<std::string>();
            if (!verifyPassword(password, storedHash)) {
                callback(errorResponse("Invalid password", k401Unauthorized));
                return;
            }

            auto accessToken = generateAccessToken(username);
            auto refreshToken = generateRefreshToken(username);

            Json::Value resp;
            resp["status"] = "success";
            resp["access_token"] = accessToken;
            resp["refresh_token"] = refreshToken;
            callback(HttpResponse::newHttpJsonResponse(resp));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            callback(errorResponse("Error logging in", k500InternalServerError));
        },
        username
    );
}

// ---------------------- JWT Tokens ----------------------
std::string AuthController::generateAccessToken(const std::string &username)
{
    using namespace std::chrono;
    return jwt::create()
        .set_issuer("my_cppAuth")
        .set_subject(username)
        .set_issued_at(system_clock::now())
        .set_expires_at(system_clock::now() + minutes{15})
        .sign(jwt::algorithm::hs256{jwtSecret});
}

std::string AuthController::generateRefreshToken(const std::string &username)
{
    using namespace std::chrono;
    auto token = jwt::create()
        .set_issuer("my_cppAuth")
        .set_subject(username)
        .set_issued_at(system_clock::now())
        .set_expires_at(system_clock::now() + hours{24*7})
        .sign(jwt::algorithm::hs256{jwtSecret});

    std::lock_guard<std::mutex> lock(refreshMutex);
    refreshTokens[username] = token;
    return token;
}

// ---------------------- Refresh Token ----------------------
void AuthController::refreshToken(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonReq = validateJson(req);
    if (!jsonReq) {
        callback(errorResponse("Invalid JSON", k400BadRequest));
        return;
    }

    std::string username = (*jsonReq)["username"].asString();
    std::string oldToken = (*jsonReq)["refresh_token"].asString();

    std::lock_guard<std::mutex> lock(refreshMutex);
    if (refreshTokens.find(username) == refreshTokens.end() || refreshTokens[username] != oldToken) {
        callback(errorResponse("Invalid refresh token", k401Unauthorized));
        return;
    }

    try {
        auto decoded = jwt::decode(oldToken);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwtSecret})
            .with_issuer("my_cppAuth")
            .verify(decoded);

        Json::Value resp;
        resp["access_token"] = generateAccessToken(username);
        resp["refresh_token"] = generateRefreshToken(username);
        callback(HttpResponse::newHttpJsonResponse(resp));
    }
    catch (...) {
        callback(errorResponse("Refresh token expired or invalid", k401Unauthorized));
    }
}

// ---------------------- Get Profile ----------------------
void AuthController::getProfile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string username;
    try {
        username = req->attributes()->get<std::string>("username");
    } catch (...) {
        callback(errorResponse("Username attribute missing", k401Unauthorized));
        return;
    }

    dbClient->execSqlAsync(
        "SELECT id, username, email, created_at FROM users WHERE username=$1",
        [callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(errorResponse("User not found", k404NotFound));
                return;
            }
            Json::Value respJson;
            respJson["id"] = r[0]["id"].as<int>();
            respJson["username"] = r[0]["username"].as<std::string>();
            respJson["email"] = r[0]["email"].as<std::string>();
            respJson["created_at"] = r[0]["created_at"].as<std::string>();
            callback(HttpResponse::newHttpJsonResponse(respJson));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            callback(errorResponse("Database error", k500InternalServerError));
        },
        username
    );
}
