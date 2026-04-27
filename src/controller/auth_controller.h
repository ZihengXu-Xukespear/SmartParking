#pragma once
#include "base_controller.h"
#include "../service/auth_service.h"
#include "../model/user.h"

class AuthController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/auth"; }

    void registerRoutes(crow::SimpleApp& app) override {
        // Login
        CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            std::string username = body["username"].s();
            std::string password = body["password"].s();

            if (username.empty() || password.empty())
                return BaseController::errorResponse(400, "用户名和密码不能为空");

            User user;
            std::string token = AuthService::instance().login(username, password, user);
            if (token.empty())
                return BaseController::errorResponse(401, "用户名或密码错误");

            crow::json::wvalue res;
            res["token"] = token;
            res["user"] = user.serialize();
            return crow::response(res);
        });

        // Register
        CROW_ROUTE(app, "/api/auth/register").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            User user;
            user.username = body["username"].s();
            user.password = body["password"].s();
            user.telephone = body["telephone"].s();
            user.truename = body["truename"].s();

            if (user.username.empty() || user.password.empty())
                return BaseController::errorResponse(400, "用户名和密码不能为空");

            // Check password match
            if (body.has("confirm_password") && body["confirm_password"].s() != user.password)
                return BaseController::errorResponse(400, "两次密码不一致");

            user.role = "user";
            if (!AuthService::instance().registerUser(user))
                return BaseController::errorResponse(400, "注册失败，用户名可能已存在");

            return BaseController::successResponse("注册成功");
        });

        // Logout
        CROW_ROUTE(app, "/api/auth/logout").methods("POST"_method)([](const crow::request& req) {
            std::string token = BaseController::extractToken(req);
            AuthService::instance().logout(token);
            return BaseController::successResponse("已退出登录");
        });

        // Check token
        CROW_ROUTE(app, "/api/auth/check").methods("GET"_method)([](const crow::request& req) {
            std::string token = BaseController::extractToken(req);
            if (!AuthService::instance().validateToken(token))
                return BaseController::errorResponse(401, "Token无效");
            return crow::response(200, "{\"valid\":true}");
        });
    }
};
