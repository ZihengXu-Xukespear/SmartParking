#pragma once
#include "base_controller.h"
#include "../service/plate_service.h"

class PlateController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/plate"; }

    void registerRoutes(crow::SimpleApp& app) override {
        CROW_ROUTE(app, "/api/plate/recognize").methods("POST"_method)([](const crow::request& req) {
            if (!BaseController::checkPermission(req, Permissions::PLATE_RECOGNIZE))
                return BaseController::errorResponse(403, "权限不足");
            auto result = PlateService::instance().recognize(req.body);

            crow::json::wvalue res;
            res["plate_number"] = result.plate_number;
            res["confidence"] = result.confidence;
            res["color"] = result.color;
            res["message"] = result.message;
            res["stub"] = true;
            return crow::response(res);
        });

        CROW_ROUTE(app, "/api/plate/validate").methods("POST"_method)([](const crow::request& req) {
            // Format validation is available to all authenticated users
            if (!BaseController::isAuthenticated(req))
                return BaseController::errorResponse(401, "请先登录");
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            std::string plate = body["license_plate"].s();
            bool valid = PlateService::validatePlate(plate);

            crow::json::wvalue res;
            res["valid"] = valid;
            res["plate"] = plate;
            if (!valid) res["message"] = "车牌号格式不正确，应为：省份汉字+字母+5-6位数字/字母";
            return crow::response(res);
        });
    }
};
