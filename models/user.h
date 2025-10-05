#pragma once
#include <drogon/orm/Mapper.h>
#include <string>

class User {
public:
    int id;
    std::string username;
    std::string email;
    std::string password_hash;
};
