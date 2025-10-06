#pragma once

#include <string>

// Minimal jwt helper stub used by BankController during builds.
// verifyJWT should check token validity and return true/false; here it's a stub.
inline bool verifyJWT(const std::string &token, const std::string &secret, std::string &outUsername) {
    // naive stub: accept any non-empty token and set username to 'stub'
    if(token.empty()) return false;
    outUsername = "stubuser";
    return true;
}
