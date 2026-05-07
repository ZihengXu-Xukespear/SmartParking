#include "balance_controller.h"
#include "../service/balance_service.h"
#include "../permissions.h"

std::string BalanceController::getPrefix() const { return "/api/balance"; }

void BalanceController::registerRoutes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/balance").methods("GET"_method)([](const crow::request& req) {
        if (!BaseController::checkPermission(req, Permissions::BALANCE_VIEW))
            return BaseController::errorResponse(403, "权限不足");
        auto auth = BaseController::authenticate(req);
        int userId = auth.first;

        double balance = BalanceService::instance().getBalance(userId);
        auto transactions = BalanceService::instance().getTransactions(userId, 20);

        crow::json::wvalue res;
        res["balance"] = balance;
        res["transactions"] = BaseController::toJsonArray(transactions);
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/balance/<int>").methods("GET"_method)([](const crow::request& req, int userId) {
        if (!BaseController::checkPermission(req, Permissions::BALANCE_MANAGE))
            return BaseController::errorResponse(403, "权限不足");

        double balance = BalanceService::instance().getBalance(userId);
        auto transactions = BalanceService::instance().getTransactions(userId, 50);

        crow::json::wvalue res;
        res["balance"] = balance;
        res["transactions"] = BaseController::toJsonArray(transactions);
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/balance/recharge").methods("POST"_method)([](const crow::request& req) {
        if (!BaseController::checkPermission(req, Permissions::BALANCE_MANAGE))
            return BaseController::errorResponse(403, "权限不足");
        auto body = BaseController::parseBody(req);
        if (!body) return BaseController::errorResponse(400, "Invalid JSON");

        int userId = body["user_id"].i();
        double amount = body["amount"].d();
        std::string desc = body.has("description") ? std::string(body["description"].s()) : "管理员充值";

        std::string error;
        if (!BalanceService::instance().recharge(userId, amount, desc, error))
            return BaseController::errorResponse(400, error);

        double newBal = BalanceService::instance().getBalance(userId);
        crow::json::wvalue res;
        res["message"] = "充值成功";
        res["balance"] = newBal;
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/balance/deposit").methods("POST"_method)([](const crow::request& req) {
        if (!BaseController::isAuthenticated(req))
            return BaseController::errorResponse(401, "请先登录");
        auto auth = BaseController::authenticate(req);
        auto body = BaseController::parseBody(req);
        if (!body) return BaseController::errorResponse(400, "Invalid JSON");

        double amount = body["amount"].d();
        if (amount < 1.0) return BaseController::errorResponse(400, "充值金额至少1元");
        if (amount > 10000) return BaseController::errorResponse(400, "单次充值最多10000元");

        std::string error;
        if (!BalanceService::instance().recharge(auth.first, amount, "自助充值", error))
            return BaseController::errorResponse(400, error);

        crow::json::wvalue res;
        res["message"] = "充值成功";
        res["balance"] = BalanceService::instance().getBalance(auth.first);
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/balance/transactions").methods("GET"_method)([](const crow::request& req) {
        if (!BaseController::checkPermission(req, Permissions::BALANCE_MANAGE))
            return BaseController::errorResponse(403, "权限不足");
        auto transactions = BalanceService::instance().getTransactions(0, 200);
        crow::json::wvalue res;
        res["transactions"] = BaseController::toJsonArray(transactions);
        return crow::response(res);
    });
}
