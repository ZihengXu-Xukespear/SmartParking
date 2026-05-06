#pragma once
#include "base_controller.h"
#include "../service/blacklist_service.h"

class BlacklistController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/blacklist"; }

    void registerRoutes(crow::SimpleApp& app) override {
        CROW_ROUTE(app, "/api/blacklist").methods("GET"_method)([](const crow::request& req) {
            if (!BaseController::checkPermission(req, Permissions::BLACKLIST_MANAGE))
                return BaseController::errorResponse(403, "权限不足");
            auto list = BlacklistService::instance().getAll();
            crow::json::wvalue res;
            res["blacklist"] = BaseController::toJsonArray(list);
            return crow::response(res);
        });

        CROW_ROUTE(app, "/api/blacklist").methods("POST"_method)([](const crow::request& req) {
            if (!BaseController::checkPermission(req, Permissions::BLACKLIST_MANAGE))
                return BaseController::errorResponse(403, "权限不足");
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");
            std::string plate = body["license_plate"].s();
            std::string reason = body.has("reason") ? std::string(body["reason"].s()) : "";
            std::string error;
            if (!BlacklistService::instance().add(plate, reason, error))
                return BaseController::errorResponse(400, error);
            return BaseController::successResponse("已添加至黑名单");
        });

        CROW_ROUTE(app, "/api/blacklist/<int>").methods("DELETE"_method)([](const crow::request& req, int id) {
            if (!BaseController::checkPermission(req, Permissions::BLACKLIST_MANAGE))
                return BaseController::errorResponse(403, "权限不足");
            if (!BlacklistService::instance().remove(id))
                return BaseController::errorResponse(400, "移除失败");
            return BaseController::successResponse("已移除");
        });
    }
};
