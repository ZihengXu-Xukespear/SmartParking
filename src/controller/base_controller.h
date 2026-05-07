#pragma once
#include "crow.h"
#include <string>
#include <utility>
#include "../permissions.h"

class BaseController {
public:
    virtual ~BaseController() = default;

    virtual void registerRoutes(crow::SimpleApp& app) = 0;
    virtual std::string getPrefix() const = 0;

public:
    BaseController() = default;

    // Helper: convert a vector of serializable models to a JSON array
    template <typename T>
    static std::vector<crow::json::wvalue> toJsonArray(const std::vector<T>& items) {
        std::vector<crow::json::wvalue> arr;
        for (const auto& item : items) arr.push_back(item.serialize());
        return arr;
    }

    // Helper: wrap a JSON array in a response with a key name
    template <typename T>
    static crow::response jsonResponse(const std::string& key, const std::vector<T>& items) {
        crow::json::wvalue res;
        res[key] = toJsonArray(items);
        crow::response r(res);
        r.set_header("Content-Type", "application/json; charset=utf-8");
        return r;
    }

    static crow::json::wvalue jsonValue(double v) { return crow::json::wvalue(v); }
    static crow::json::wvalue jsonValue(int v) { return crow::json::wvalue(v); }
    static crow::json::wvalue jsonValue(const std::string& v) { return crow::json::wvalue(v); }

    // Build a JSON array from a vector of strings
    static std::vector<crow::json::wvalue> toJsonArray(const std::vector<std::string>& items) {
        std::vector<crow::json::wvalue> arr;
        for (const auto& item : items) arr.push_back(crow::json::wvalue(item));
        return arr;
    }

    static crow::response errorResponse(int code, const std::string& message) {
        crow::response res(code, "{\"error\":\"" + message + "\"}");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        return res;
    }

    static crow::response successResponse(const std::string& message) {
        crow::response res(200, "{\"message\":\"" + message + "\"}");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        return res;
    }

    // Permission and role checks (defined after AuthService include)
    static bool checkPermission(const crow::request& req, const std::string& permission);
    static bool checkPermissions(const crow::request& req, const std::vector<std::string>& permissions);
    static bool isAdmin(const crow::request& req);
    static bool isRoot(const crow::request& req);

    static crow::json::rvalue parseBody(const crow::request& req) {
        return crow::json::load(req.body);
    }

    static bool isAuthenticated(const crow::request& req) {
        return authenticate(req).first != -1;
    }

protected:
    static std::string extractToken(const crow::request& req) {
        std::string token = req.get_header_value("Authorization");
        if (token.size() >= 7 && token.substr(0, 7) == "Bearer ") token = token.substr(7);
        return token;
    }

    static std::pair<int, std::string> authenticate(const crow::request& req);

    static std::string getRole(const crow::request& req) {
        return authenticate(req).second;
    }
};

#include "../service/auth_service.h"

inline std::pair<int, std::string> BaseController::authenticate(const crow::request& req) {
    std::string token = extractToken(req);
    if (token.empty()) return {-1, ""};
    int uid = AuthService::instance().getUserId(token);
    if (uid == -1) return {-1, ""};
    std::string role = AuthService::instance().getUserRole(token);
    return {uid, role};
}

inline bool BaseController::checkPermission(const crow::request& req, const std::string& permission) {
    std::string token = extractToken(req);
    if (token.empty()) return false;
    return AuthService::instance().hasPermission(token, permission);
}

inline bool BaseController::checkPermissions(const crow::request& req, const std::vector<std::string>& permissions) {
    std::string token = extractToken(req);
    if (token.empty()) return false;
    for (const auto& p : permissions) {
        if (!AuthService::instance().hasPermission(token, p)) return false;
    }
    return true;
}

inline bool BaseController::isAdmin(const crow::request& req) {
    return checkPermission(req, Permissions::USER_MANAGE);
}

inline bool BaseController::isRoot(const crow::request& req) {
    return getRole(req) == "root";
}
