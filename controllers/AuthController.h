#pragma once
#include <drogon/HttpController.h>
#include <bcrypt/BCrypt.hpp> // Commented out because libbcrypt-dev is unavailable
#include <jwt-cpp/jwt.h>
#include <unordered_map>
#include <mutex>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/register", Post);
    ADD_METHOD_TO(AuthController::loginUser, "/login", Post);
    ADD_METHOD_TO(AuthController::refreshToken, "/refresh", Post);
    ADD_METHOD_TO(AuthController::getProfile, "/api/profile", Get);
    METHOD_LIST_END

    void registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void refreshToken(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getProfile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

private:
    std::unordered_map<std::string, std::string> refreshTokens; // username -> refresh token
    std::mutex refreshMutex;
    std::string jwtSecret = "supersecretkey"; // change this to env var in production

    std::string generateAccessToken(const std::string &username);
    std::string generateRefreshToken(const std::string &username);
};
