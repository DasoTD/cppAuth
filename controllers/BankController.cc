#include "BankController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <spdlog/spdlog.h>
#include <random>
#include "jwt_utils.h"  // your JWT helper functions

using namespace drogon;

std::string generateAccountNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000000, 99999999);
    return "ACCT" + std::to_string(dis(gen));
}

void BankController::getBalance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto authHeader = req->getHeader("Authorization");
    std::string token = authHeader.substr(7); // "Bearer <token>"
    std::string account;
    if (!verifyJWT(token, jwtSecret_, account)) {
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized");
            callback(resp);
        }
        return;
    }

    dbClient_->execSqlAsync(
        "SELECT balance FROM users WHERE account_number=$1",
        [callback, account](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("Account not found");
                callback(resp);
                spdlog::warn("Balance check failed for {}", account);
                return;
            }
            double balance = r[0]["balance"].as<double>();
            Json::Value j;
            j["balance"] = balance;
            callback(HttpResponse::newHttpJsonResponse(j));
            spdlog::info("Balance retrieved for {}", account);
            },
        [callback, account](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(std::string("Internal error: ") + e.base().what());
            callback(resp);
            spdlog::error("Balance query failed for {}: {}", account, e.base().what());
        },
        account
    );
}

void BankController::deposit(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto authHeader = req->getHeader("Authorization");
    std::string token = authHeader.substr(7);
    std::string account;
    if (!verifyJWT(token, jwtSecret_, account)) {
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized");
            callback(resp);
        }
        return;
    }

    auto json = req->getJsonObject();
    double amount = (*json)["amount"].asDouble();

    if (amount <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid amount");
        callback(resp);
        return;
    }

    dbClient_->execSqlAsync(
        "UPDATE users SET balance = balance + $1 WHERE account_number=$2 RETURNING balance",
        [callback, account, amount](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k404NotFound);
                resp->setBody("Account not found");
                callback(resp);
                spdlog::warn("Deposit failed for {}", account);
                return;
            }
            double newBalance = r[0]["balance"].as<double>();
            Json::Value j;
            j["new_balance"] = newBalance;
            callback(HttpResponse::newHttpJsonResponse(j));
            spdlog::info("Deposited {} to {}, new balance {}", amount, account, newBalance);
        },
        [callback, account](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(std::string("Internal error: ") + e.base().what());
            callback(resp);
            spdlog::error("Deposit failed for {}: {}", account, e.base().what());
        },
        amount, account
    );
}

void BankController::withdraw(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto authHeader = req->getHeader("Authorization");
    std::string token = authHeader.substr(7);
    std::string account;
    if (!verifyJWT(token, jwtSecret_, account)) {
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized");
            callback(resp);
        }
        return;
    }

    auto json = req->getJsonObject();
    double amount = (*json)["amount"].asDouble();

    dbClient_->execSqlAsync(
        "UPDATE users SET balance = balance - $1 WHERE account_number=$2 AND balance >= $1 RETURNING balance",
        [callback, account, amount](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("Insufficient balance");
                callback(resp);
                spdlog::warn("Withdraw failed for {}: insufficient balance", account);
                return;
            }
            double newBalance = r[0]["balance"].as<double>();
            Json::Value j;
            j["new_balance"] = newBalance;
            callback(HttpResponse::newHttpJsonResponse(j));
            spdlog::info("Withdrew {} from {}, new balance {}", amount, account, newBalance);
        },
        [callback, account](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(std::string("Internal error: ") + e.base().what());
            callback(resp);
            spdlog::error("Withdraw failed for {}: {}", account, e.base().what());
        },
        amount, account
    );
}

void BankController::transfer(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto authHeader = req->getHeader("Authorization");
    std::string token = authHeader.substr(7);
    std::string fromAccount;
    if (!verifyJWT(token, jwtSecret_, fromAccount)) {
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Unauthorized");
            callback(resp);
        }
        return;
    }

    auto json = req->getJsonObject();
    std::string toAccount = (*json)["to_account"].asString();
    double amount = (*json)["amount"].asDouble();

    if (amount <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid amount");
        callback(resp);
        return;
    }

    dbClient_->execSqlAsync(
        "BEGIN;"
        "UPDATE users SET balance = balance - $1 WHERE account_number=$2 AND balance >= $1;"
        "UPDATE users SET balance = balance + $1 WHERE account_number=$3;"
        "COMMIT;",
        [callback, fromAccount, toAccount, amount](const drogon::orm::Result &r) {
            Json::Value j;
            j["status"] = "success";
            callback(HttpResponse::newHttpJsonResponse(j));
            spdlog::info("Transferred {} from {} to {}", amount, fromAccount, toAccount);
        },
        [callback, fromAccount](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(std::string("Internal error: ") + e.base().what());
            callback(resp);
            spdlog::error("Transfer failed for {}: {}", fromAccount, e.base().what());
        },
        amount, fromAccount, toAccount
    );
}


// #include "BankController.h"
// #include "DbLogger.h"
// #include <json/json.h>
// #include <spdlog/spdlog.h>

// orm::DbClientPtr BankController::dbClient_ = nullptr;

// void BankController::getBalance(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string &account)
// {
//     std::string sql = "SELECT balance FROM accounts WHERE account_number=$1;";
//     spdlog::info("Getting balance for account {}", account);

//     dbClient_->execSqlAsync(sql,
//         [callback](const orm::Result &r){
//             Json::Value resp;
//             if(!r.empty()) resp["balance"] = r[0]["balance"].as<double>();
//             else resp["error"] = "Account not found";
//             callback(HttpResponse::newHttpJsonResponse(resp));
//         },
//         [callback](const std::exception &e){
//             Json::Value resp;
//             resp["error"] = e.what();
//             callback(HttpResponse::newHttpJsonResponse(resp));
//         },
//         account
//     );
// }

// void BankController::withdraw(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string &account)
// {
//     auto json = req->getJsonObject();
//     if(!json || !(*json)["amount"].isNumeric())
//     {
//         callback(HttpResponse::newHttpResponse());
//         return;
//     }
//     double amount = (*json)["amount"].asDouble();
//     std::string sql = "UPDATE accounts SET balance = balance - $1 WHERE account_number=$2 AND balance >= $1 RETURNING balance;";
//     spdlog::info("Withdrawing {} from account {}", amount, account);

//     dbClient_->execSqlAsync(sql,
//         [callback](const orm::Result &r){
//             Json::Value resp;
//             if(!r.empty()) resp["balance"] = r[0]["balance"].as<double>();
//             else resp["error"] = "Insufficient funds or account not found";
//             callback(HttpResponse::newHttpJsonResponse(resp));
//         },
//         [callback](const std::exception &e){
//             Json::Value resp;
//             resp["error"] = e.what();
//             callback(HttpResponse::newHttpJsonResponse(resp));
//         },
//         amount, account
//     );
// }

// void BankController::credit(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string &account)
// {
//     auto json = req->getJsonObject();
//     if(!json || !(*json)["amount"].isNumeric())
//     {
//         callback(HttpResponse::newHttpResponse());
//         return;
//     }
//     double amount = (*json)["amount"].asDouble();
//     std::string sql = "UPDATE accounts SET balance = balance + $1 WHERE account_number=$2 RETURNING balance;";
//     spdlog::info("Crediting {} to account {}", amount, account);

//     dbClient_->execSqlAsync(sql,
//         [callback](const orm::Result &r){
//             Json::Value resp;
//             if(!r.empty()) resp["balance"] = r[0]["balance"].as<double>();
//             else resp["error"] = "Account not found";
//             callback(HttpResponse::newHttpJsonResponse(resp));
//         },
//         [callback](const std::exception &e){
//             Json::Value resp;
//             resp["error"] = e.what();
//             callback(HttpResponse::newHttpJsonResponse(resp));
//         },
//         amount, account
//     );
// }

// void BankController::getBalance(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &accountNumber) {
//     dbClient_->execSqlAsync(
//         "SELECT balance FROM accounts WHERE account_number=$1;",
//         [callback](const orm::Result &r) {
//             if (r.empty()) {
//                 callback(HttpResponse::newHttpJsonResponse(Json::Value("Account not found")));
//                 return;
//             }
//             Json::Value res;
//             res["balance"] = r[0]["balance"].as<double>();
//             callback(HttpResponse::newHttpJsonResponse(res));
//         },
//         [callback](const std::exception &e) {
//             callback(HttpResponse::newHttpJsonResponse(Json::Value(std::string("Error: ") + e.what())));
//         },
//         accountNumber
//     );
// }

// void BankController::deposit(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string &accountNumber, double amount) {
//     dbClient_->execSqlAsync(
//         "UPDATE accounts SET balance = balance + $1 WHERE account_number=$2 RETURNING balance;",
//         [callback](const orm::Result &r) {
//             Json::Value res;
//             res["new_balance"] = r[0]["balance"].as<double>();
//             callback(HttpResponse::newHttpJsonResponse(res));
//         },
//         [callback](const std::exception &e) {
//             callback(HttpResponse::newHttpJsonResponse(Json::Value(std::string("Deposit failed: ") + e.what())));
//         },
//         amount, accountNumber
//     );
// }


// void BankController::transfer(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback)
// {
//     auto json = req->getJsonObject();
//     if(!json || !(*json)["from"].isString() || !(*json)["to"].isString() || !(*json)["amount"].isNumeric())
//     {
//         callback(HttpResponse::newHttpJsonResponse(Json::Value("Invalid request")));
//         return;
//     }

//     std::string from = (*json)["from"].asString();
//     std::string to = (*json)["to"].asString();
//     double amount = (*json)["amount"].asDouble();

//     spdlog::info("Attempting atomic transfer of {} from {} to {}", amount, from, to);

//     dbClient_->execSqlAsync(
//         "BEGIN;",
//         [](const orm::Result &){}, // Begin transaction
//         [callback](const std::exception &e){
//             callback(HttpResponse::newHttpJsonResponse(Json::Value(std::string("Transaction failed: ") + e.what())));
//         }
//     );

//     // Withdraw from sender
//     std::string withdrawSql = "UPDATE accounts SET balance = balance - $1 WHERE account_number=$2 AND balance >= $1 RETURNING balance;";
//     dbClient_->execSqlAsync(
//         withdrawSql,
//         [callback, to, amount, this](const orm::Result &r){
//             if(r.empty())
//             {
//                 spdlog::warn("Insufficient funds or sender account not found");
//                 dbClient_->execSqlAsync("ROLLBACK;", [](const orm::Result &){}, [](const std::exception &){});
//                 callback(HttpResponse::newHttpJsonResponse(Json::Value("Insufficient funds or sender not found")));
//                 return;
//             }

//             // Credit receiver
//             std::string creditSql = "UPDATE accounts SET balance = balance + $1 WHERE account_number=$2 RETURNING balance;";
//             dbClient_->execSqlAsync(
//                 creditSql,
//                 [callback](const orm::Result &r2){
//                     spdlog::info("Transfer completed successfully");
//                     dbClient_->execSqlAsync("COMMIT;", [](const orm::Result &){}, [](const std::exception &){});
//                     callback(HttpResponse::newHttpJsonResponse(Json::Value("Transfer successful")));
//                 },
//                 [callback](const std::exception &e2){
//                     spdlog::error("Failed to credit receiver: {}", e2.what());
//                     dbClient_->execSqlAsync("ROLLBACK;", [](const orm::Result &){}, [](const std::exception &){});
//                     callback(HttpResponse::newHttpJsonResponse(Json::Value(std::string("Transfer failed: ") + e2.what())));
//                 },
//                 amount, to
//             );

//         },
//         [callback](const std::exception &e){
//             spdlog::error("Failed to withdraw from sender: {}", e.what());
//             dbClient_->execSqlAsync("ROLLBACK;", [](const orm::Result &){}, [](const std::exception &){});
//             callback(HttpResponse::newHttpJsonResponse(Json::Value(std::string("Transfer failed: ") + e.what())));
//         },
//         amount, from
//     );
// }


// void BankController::transfer(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback)
// {
//     auto json = req->getJsonObject();
//     if(!json || !(*json)["from"].isString() || !(*json)["to"].isString() || !(*json)["amount"].isNumeric())
//     {
//         callback(HttpResponse::newHttpResponse());
//         return;
//     }
//     std::string from = (*json)["from"].asString();
//     std::string to = (*json)["to"].asString();
//     double amount = (*json)["amount"].asDouble();

//     spdlog::info("Transferring {} from {} to {}", amount, from, to);

//     dbClient_->execSqlAsync("BEGIN;", [](const orm::Result &){}, [](const std::exception &e){});
//     dbClient_->execSqlAsync("UPDATE accounts SET balance = balance - $1 WHERE account_number=$2 AND balance >= $1 RETURNING balance;", 
//         [callback, from](const orm::Result &r){
//             if(r.empty())
//             {
//                 callback(HttpResponse::newHttpJsonResponse(Json::Value("Insufficient funds or from-account not found")));
//                 return;
//             }
//         }, 
//         [callback](const std::exception &e){
//             callback(HttpResponse::newHttpJsonResponse(Json::Value(e.what())));
//         },
//         amount, from
//     );
//     dbClient_->execSqlAsync("UPDATE accounts SET balance = balance + $1 WHERE account_number=$2;", 
//         [callback, to](const orm::Result &r){
//             callback(HttpResponse::newHttpJsonResponse(Json::Value("Transfer completed")));
//         },
//         [callback](const std::exception &e){
//             callback(HttpResponse::newHttpJsonResponse(Json::Value(e.what())));
//         },
//         amount, to
//     );
//     dbClient_->execSqlAsync("COMMIT;", [](const orm::Result &){}, [](const std::exception &e){});
// }
