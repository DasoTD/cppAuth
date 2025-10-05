#include "AuthController.h"
#include <drogon/orm/DbClient.h>
#include <drogon/orm/DbTypes.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <drogon/drogon.h>
#include <string>
#include <functional>
#include "AuthController.h"

using json = nlohmann::json;

extern drogon::orm::DbClientPtr dbClient; // Initialized in main.cpp

// ---------------------- Helper Functions ----------------------
namespace {
    // Returns nullptr if JSON is invalid
    std::shared_ptr<const drogon::HttpRequestJson> validateJson(const HttpRequestPtr &req) {
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

AuthController::AuthController() {
    const char *envSecret = std::getenv("JWT_SECRET");
    if (envSecret) {
        jwtSecret = envSecret;
        std::cout << "[INFO] Loaded JWT_SECRET from environment." << std::endl;
    } else {
        jwtSecret = "supersecretkey"; // fallback
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
            callback(HttpResponse::newHttpJsonResponse({{"status", "success"}}));
        },
        [callback](const std::exception &) {
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

            callback(HttpResponse::newHttpJsonResponse({
                {"status", "success"},
                {"access_token", accessToken},
                {"refresh_token", refreshToken}
            }));
        },
        [callback](const std::exception &) {
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
        .set_issued_at(system_clock::to_time_t(system_clock::now()))
        .set_expires_at(system_clock::to_time_t(system_clock::now() + minutes{15}))
        .sign(jwt::algorithm::hs256{jwtSecret});
}

std::string AuthController::generateRefreshToken(const std::string &username)
{
    using namespace std::chrono;
    auto token = jwt::create()
        .set_issuer("my_cppAuth")
        .set_subject(username)
        .set_issued_at(system_clock::to_time_t(system_clock::now()))
        .set_expires_at(system_clock::to_time_t(system_clock::now() + hours{24*7}))
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

        callback(HttpResponse::newHttpJsonResponse({
            {"access_token", generateAccessToken(username)},
            {"refresh_token", generateRefreshToken(username)}
        }));
    }
    catch (...) {
        callback(errorResponse("Refresh token expired or invalid", k401Unauthorized));
    }
}

// ---------------------- Get Profile ----------------------
void AuthController::getProfile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string username = req->getAttribute<std::string>("username"); // set by middleware

    dbClient->execSqlAsync(
        "SELECT id, username, email, created_at FROM users WHERE username=$1",
        [callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(errorResponse("User not found", k404NotFound));
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
        [callback](const std::exception &) {
            callback(errorResponse("Database error", k500InternalServerError));
        },
        username
    );
}
