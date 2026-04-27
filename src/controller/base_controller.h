#pragma once
#include "crow.h"
#include <string>

class BaseController {
public:
    virtual ~BaseController() = default;

    virtual void registerRoutes(crow::SimpleApp& app) = 0;
    virtual std::string getPrefix() const = 0;

protected:
    BaseController() = default;

    static crow::response errorResponse(int code, const std::string& message) {
        return crow::response(code, "{\"error\":\"" + message + "\"}");
    }

    static crow::response successResponse(const std::string& message) {
        return crow::response(200, "{\"message\":\"" + message + "\"}");
    }

    static crow::json::rvalue parseBody(const crow::request& req) {
        return crow::json::load(req.body);
    }

    static std::string extractToken(const crow::request& req) {
        std::string token = req.get_header_value("Authorization");
        if (token.size() >= 7 && token.substr(0, 7) == "Bearer ") token = token.substr(7);
        return token;
    }
};
