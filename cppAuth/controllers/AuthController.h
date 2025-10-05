#pragma once
#include <drogon/HttpController.h>
#include <bcrypt/BCrypt.hpp>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/register", Post);
    ADD_METHOD_TO(AuthController::loginUser, "/login", Post);
    METHOD_LIST_END

    void registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void loginUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};
