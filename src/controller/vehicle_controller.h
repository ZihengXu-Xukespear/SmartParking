#pragma once
#include "base_controller.h"
#include "../service/vehicle_service.h"
#include "../service/auth_service.h"

class VehicleController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/vehicle"; }

    void registerRoutes(crow::SimpleApp& app) override {
        // Vehicle check-in
        CROW_ROUTE(app, "/api/vehicle/checkin").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            std::string plate = body["license_plate"].s();
            std::string billing_type = "standard";
            if (body.has("billing_type")) billing_type = body["billing_type"].s();

            std::string error;
            if (!VehicleService::instance().checkIn(plate, billing_type, error))
                return BaseController::errorResponse(400, error);

            crow::json::wvalue res;
            res["message"] = "车辆入库成功";
            res["license_plate"] = plate;
            return crow::response(res);
        });

        // Vehicle check-out
        CROW_ROUTE(app, "/api/vehicle/checkout").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            std::string plate = body["license_plate"].s();
            double fee = 0;
            CarRecord record;
            std::string error;

            if (!VehicleService::instance().checkOut(plate, fee, record, error))
                return BaseController::errorResponse(400, error);

            crow::json::wvalue res;
            res["message"] = "车辆出库成功";
            res["fee"] = fee;
            res["record"] = record.serialize();
            return crow::response(res);
        });

        // Query records
        CROW_ROUTE(app, "/api/vehicle/query").methods("GET"_method)([](const crow::request& req) {
            std::string plate = req.url_params.get("plate") ? req.url_params.get("plate") : "";
            std::string start = req.url_params.get("start") ? req.url_params.get("start") : "";
            std::string end = req.url_params.get("end") ? req.url_params.get("end") : "";

            auto records = VehicleService::instance().queryRecords(plate, start, end);
            crow::json::wvalue res;
            std::vector<crow::json::wvalue> arr;
            for (auto& r : records) {
                arr.push_back(r.serialize());
            }
            res["records"] = std::move(arr);
            res["total"] = (int)records.size();
            return crow::response(res);
        });

        // Delete record
        CROW_ROUTE(app, "/api/vehicle/<int>").methods("DELETE"_method)([](int id) {
            if (!VehicleService::instance().deleteRecord(id))
                return BaseController::errorResponse(400, "删除失败");
            return BaseController::successResponse("删除成功");
        });
    }
};
