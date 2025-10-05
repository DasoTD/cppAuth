#pragma once
#include <drogon/HttpFilter.h>
#include <jwt-cpp/jwt.h>

using namespace drogon;

class JwtMiddleware : public HttpFilter<JwtMiddleware> {
public:
    JwtMiddleware() {
        jwtSecret = "supersecretkey"; // same as in AuthController; move to env var in production
    }

    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override {

        // Get Authorization header
        auto authHeader = req->getHeader("Authorization");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            auto resp = HttpResponse::newHttpResponse(k401Unauthorized);
            resp->setBody("Missing or invalid Authorization header");
            fcb(resp);
            return;
        }

        std::string token = authHeader.substr(7); // skip "Bearer "

        try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{jwtSecret})
                .with_issuer("my_drogon_app");
            verifier.verify(decoded);

            // Pass username to request attributes
            req->attributes()["username"] = decoded.get_subject();
            
            fccb(); // proceed to the controller
        } catch (const std::exception &e) {
            auto resp = HttpResponse::newHttpResponse(k401Unauthorized);
            resp->setBody("Invalid or expired token");
            fcb(resp);
        }
    }

private:
    std::string jwtSecret;
};
