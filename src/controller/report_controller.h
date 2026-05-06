#pragma once
#include "base_controller.h"
#include "../service/report_service.h"

class ReportController : public BaseController {
public:
    std::string getPrefix() const override { return "/api/report"; }

    void registerRoutes(crow::SimpleApp& app) override {
        CROW_ROUTE(app, "/api/report/summary").methods("GET"_method)([](const crow::request& req) {
            if (!BaseController::checkPermission(req, Permissions::REPORT_VIEW))
                return BaseController::errorResponse(403, "权限不足");
            auto s = ReportService::instance().getSummary();
            crow::json::wvalue res;
            res["today_income"] = s.today_income;
            res["month_income"] = s.month_income;
            res["total_income"] = s.total_income;
            res["parking_fees"] = s.parking_fees;
            res["pass_sales"] = s.pass_sales;
            res["reservation_prepaid"] = s.reservation_prepaid;
            return crow::response(res);
        });

        CROW_ROUTE(app, "/api/report/daily").methods("GET"_method)([](const crow::request& req) {
            if (!BaseController::checkPermission(req, Permissions::REPORT_VIEW))
                return BaseController::errorResponse(403, "权限不足");
            auto data = ReportService::instance().getDailyRevenue(30);
            crow::json::wvalue res;
            std::vector<crow::json::wvalue> dates, values;
            for (auto& [d, v] : data) {
                dates.push_back(crow::json::wvalue(d));
                values.push_back(crow::json::wvalue(v));
            }
            res["dates"] = std::move(dates);
            res["values"] = std::move(values);
            return crow::response(res);
        });
    }
};
