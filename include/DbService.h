#pragma once
#include <drogon/orm/DbClient.h>
#include "DbLoggerJSON.h"
#include <string>
#include <functional>

class DbService
{
public:
    explicit DbService(drogon::orm::DbClientPtr client)
        : dbLogger_(client) {}

    void execSqlAsync(const std::string &sql,
                      const drogon::orm::ResultCallback &rcb,
                      const drogon::orm::ExceptionCallback &ecb = nullptr)
    {
        dbLogger_.execSqlAsync(sql, rcb, ecb);
    }

    drogon::orm::Result execSqlSync(const std::string &sql)
    {
        return dbLogger_.execSqlSync(sql);
    }

private:
    DbLoggerJSON dbLogger_;
};
