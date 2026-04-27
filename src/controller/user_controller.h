#pragma once
#include "base_controller.h"
#include "../service/user_service.h"

class UserController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/user"; }

    void registerRoutes(crow::SimpleApp& app) override {
        // List users
        CROW_ROUTE(app, "/api/user/list").methods("GET"_method)([]() {
            auto users = UserService::instance().listUsers();
            crow::json::wvalue res;
            std::vector<crow::json::wvalue> arr;
            for (auto& u : users) {
                arr.push_back(u.serialize());
            }
            res["users"] = std::move(arr);
            return crow::response(res);
        });

        // Add user
        CROW_ROUTE(app, "/api/user/add").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            User user;
            user.username = body["username"].s();
            user.password = body["password"].s();
            user.telephone = body["telephone"].s();
            user.truename = body["truename"].s();
            user.role = body.has("role") ? std::string(body["role"].s()) : std::string("user");

            if (!UserService::instance().addUser(user))
                return BaseController::errorResponse(400, "添加失败，用户名可能已存在");

            return BaseController::successResponse("添加成功");
        });

        // Update user
        CROW_ROUTE(app, "/api/user/update").methods("PUT"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            int id = body["id"].i();
            std::string telephone = body["telephone"].s();
            std::string truename = body["truename"].s();
            std::string role = body["role"].s();

            if (!UserService::instance().updateUser(id, telephone, truename, role))
                return BaseController::errorResponse(400, "更新失败");

            // Update password if provided
            if (body.has("password") && std::string(body["password"].s()).size() > 0) {
                UserService::instance().updateUserPassword(id, body["password"].s());
            }

            return BaseController::successResponse("更新成功");
        });

        // Delete user
        CROW_ROUTE(app, "/api/user/<int>").methods("DELETE"_method)([](int id) {
            if (!UserService::instance().deleteUser(id))
                return BaseController::errorResponse(400, "删除失败");
            return BaseController::successResponse("删除成功");
        });
    }
};
