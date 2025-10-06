#include <drogon/HttpController.h>
using namespace drogon;

class BankController : public drogon::HttpController<BankController, false> {
public:
    BankController(drogon::orm::DbClientPtr db, const std::string &secret)
        : dbClient_(db), jwtSecret_(secret) {}

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BankController::getBalance, "/balance", Get);
    ADD_METHOD_TO(BankController::deposit, "/deposit", Post);
    ADD_METHOD_TO(BankController::withdraw, "/withdraw", Post);
    ADD_METHOD_TO(BankController::transfer, "/transfer", Post);
    METHOD_LIST_END

    void getBalance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deposit(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void withdraw(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void transfer(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

private:
    drogon::orm::DbClientPtr dbClient_;
    std::string jwtSecret_;
};



// #pragma once
// #include <drogon/HttpController.h>
// #include <drogon/orm/DbClient.h>

// using namespace drogon;

// class BankController : public HttpController<BankController>
// {
// public:
//     METHOD_LIST_BEGIN
//     ADD_METHOD_TO(BankController::createUser, "/user/create", Post);
//     ADD_METHOD_TO(BankController::getBalance, "/user/balance/{account}", Get);
//     ADD_METHOD_TO(BankController::credit, "/user/credit/{account}", Post);
//     ADD_METHOD_TO(BankController::withdraw, "/user/withdraw/{account}", Post);
//     ADD_METHOD_TO(BankController::transfer, "/user/transfer", Post);
//     METHOD_LIST_END

//     static void setDbClient(orm::DbClientPtr client) { dbClient_ = client; }

//     void createUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
//     void getBalance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string account);
//     void credit(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string account);
//     void withdraw(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string account);
//     void transfer(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

// private:
//     static orm::DbClientPtr dbClient_;
// };
