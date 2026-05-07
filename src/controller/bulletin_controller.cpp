#include "bulletin_controller.h"
#include "../config.h"
#include "../permissions.h"

std::string BulletinController::getPrefix() const { return "/api/bulletin"; }

void BulletinController::registerRoutes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/bulletin").methods("GET"_method)([](const crow::request& req) {
        if (!BaseController::isAuthenticated(req))
            return BaseController::errorResponse(401, "请先登录");
        crow::json::wvalue res;
        res["notice"] = AppConfig::instance().notice;
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/bulletin").methods("PUT"_method)([](const crow::request& req) {
        if (!BaseController::checkPermission(req, Permissions::NOTICE_MANAGE))
            return BaseController::errorResponse(403, "权限不足");
        auto body = BaseController::parseBody(req);
        if (!body) return BaseController::errorResponse(400, "Invalid JSON");
        AppConfig::instance().notice = body["notice"].s();
        AppConfig::instance().save(AppConfig::instance().config_file);
        return BaseController::successResponse("公告已更新");
    });
}
