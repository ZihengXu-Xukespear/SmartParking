#pragma once
#include "base_controller.h"
#include "../service/parking_service.h"
#include "../service/billing_service.h"
#include "../config.h"

class ParkingController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/parking"; }

    void registerRoutes(crow::SimpleApp& app) override {
        // Get parking status
        CROW_ROUTE(app, "/api/parking/status").methods("GET"_method)([]() {
            auto lot = ParkingService::instance().getStatus();
            return crow::response(lot.serialize());
        });

        // Get stats for pie chart
        CROW_ROUTE(app, "/api/parking/stats").methods("GET"_method)([]() {
            auto lot = ParkingService::instance().getStatus();
            crow::json::wvalue res;
            res["reserved"] = lot.P_reserve_count;
            res["occupied"] = lot.P_current_count;
            res["available"] = lot.P_total_count - lot.P_current_count - lot.P_reserve_count;
            res["total"] = lot.P_total_count;
            return crow::response(res);
        });

        // Update parking settings
        CROW_ROUTE(app, "/api/parking/settings").methods("PUT"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            double fee = body["fee"].d();
            int total = body["capacity"].i();

            if (!ParkingService::instance().updateSettings(fee, total))
                return BaseController::errorResponse(400, "更新失败");

            // Also save to config
            AppConfig::instance().fee = fee;
            AppConfig::instance().capacity = total;

            return BaseController::successResponse("更新成功");
        });

        // Get billing rules
        CROW_ROUTE(app, "/api/parking/billing-rules").methods("GET"_method)([]() {
            auto rules = BillingService::instance().getRules();
            crow::json::wvalue res;
            std::vector<crow::json::wvalue> arr;
            for (auto& r : rules) {
                arr.push_back(r.serialize());
            }
            res["rules"] = std::move(arr);
            return crow::response(res);
        });

        // Update billing rule
        CROW_ROUTE(app, "/api/parking/billing-rules/<int>").methods("PUT"_method)([](const crow::request& req, int id) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            BillingRule rule;
            rule.rule_name = body["rule_name"].s();
            rule.rule_type = body["rule_type"].s();
            rule.free_minutes = body["free_minutes"].i();
            rule.hourly_rate = body["hourly_rate"].d();
            rule.max_daily_fee = body["max_daily_fee"].d();
            rule.description = body.has("description") ? std::string(body["description"].s()) : std::string("");
            rule.is_active = body["is_active"].b();

            if (!BillingService::instance().updateRule(id, rule))
                return BaseController::errorResponse(400, "更新失败");

            return BaseController::successResponse("更新成功");
        });

        // Monthly passes
        CROW_ROUTE(app, "/api/parking/monthly-passes").methods("GET"_method)([]() {
            auto passes = BillingService::instance().getMonthlyPasses();
            crow::json::wvalue res;
            std::vector<crow::json::wvalue> arr;
            for (auto& p : passes) {
                arr.push_back(p.serialize());
            }
            res["passes"] = std::move(arr);
            return crow::response(res);
        });

        CROW_ROUTE(app, "/api/parking/monthly-passes").methods("POST"_method)([](const crow::request& req) {
            auto body = BaseController::parseBody(req);
            if (!body) return BaseController::errorResponse(400, "Invalid JSON");

            MonthlyPass pass;
            pass.license_plate = body["license_plate"].s();
            pass.pass_type = body["pass_type"].s();
            pass.start_date = body["start_date"].s();
            pass.end_date = body["end_date"].s();
            pass.fee = body["fee"].d();

            if (!BillingService::instance().addMonthlyPass(pass))
                return BaseController::errorResponse(400, "添加失败");

            return BaseController::successResponse("添加成功");
        });

        CROW_ROUTE(app, "/api/parking/monthly-passes/<int>").methods("DELETE"_method)([](int id) {
            if (!BillingService::instance().deleteMonthlyPass(id))
                return BaseController::errorResponse(400, "删除失败");
            return BaseController::successResponse("删除成功");
        });
    }
};
