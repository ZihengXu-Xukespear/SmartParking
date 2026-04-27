#pragma once
#include "base_controller.h"
#include "../service/reservation_service.h"
#include "../config.h"

class ReservationController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/reservation"; }

    void registerRoutes(crow::SimpleApp& app) override {
        // Create reservation
        CROW_ROUTE(app, "/api/reservation/create").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            std::string plate = body["license_plate"].s();
            std::string P_name = body.has("P_name") ? body["P_name"].s() : AppConfig::instance().parking_name;

            std::string error;
            if (!ReservationService::instance().create(plate, P_name, error))
                return BaseController::errorResponse(400, error);

            return BaseController::successResponse("预约成功，请在30分钟内到达");
        });

        // List reservations
        CROW_ROUTE(app, "/api/reservation/list").methods("GET"_method)([]() {
            auto reservations = ReservationService::instance().list();
            crow::json::wvalue res;
            std::vector<crow::json::wvalue> arr;
            for (auto& r : reservations) {
                arr.push_back(r.serialize());
            }
            res["reservations"] = std::move(arr);
            return crow::response(res);
        });

        // Cancel reservation
        CROW_ROUTE(app, "/api/reservation/<int>").methods("DELETE"_method)([](int id) {
            if (!ReservationService::instance().cancel(id))
                return BaseController::errorResponse(400, "取消失败");
            return BaseController::successResponse("取消成功");
        });
    }
};
