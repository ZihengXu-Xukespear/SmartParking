#define _WIN32_WINNT 0x0601
#define ASIO_STANDALONE
#define CROW_STANDALONE

#include "crow.h"
#include "config.h"
#include "database/db_init.h"
#include "database/mysql_pool.h"
#include "controller/auth_controller.h"
#include "controller/vehicle_controller.h"
#include "controller/parking_controller.h"
#include "controller/user_controller.h"
#include "controller/reservation_controller.h"
#include "controller/plate_controller.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

std::string findConfigPath() {
    // Look for config file in executable directory, then in project root
    std::vector<std::string> paths = {
        "config/db_config.json",
        "../config/db_config.json"
    };
    for (const auto& p : paths) {
        if (fs::exists(p)) return p;
    }
    return "";
}

void setupStaticFiles(crow::SimpleApp& app) {
    // Serve static files from frontend/ directory
    CROW_ROUTE(app, "/<string>")
    ([](const crow::request&, std::string filename) {
        std::string path = "frontend/" + filename;
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return crow::response(404, "Not Found");

        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        crow::response res(content);

        if (filename.find(".html") != std::string::npos) res.set_header("Content-Type", "text/html; charset=utf-8");
        else if (filename.find(".css") != std::string::npos) res.set_header("Content-Type", "text/css; charset=utf-8");
        else if (filename.find(".js") != std::string::npos) res.set_header("Content-Type", "application/javascript; charset=utf-8");
        else if (filename.find(".png") != std::string::npos) res.set_header("Content-Type", "image/png");
        else if (filename.find(".jpg") != std::string::npos || filename.find(".jpeg") != std::string::npos)
            res.set_header("Content-Type", "image/jpeg");
        else if (filename.find(".ico") != std::string::npos) res.set_header("Content-Type", "image/x-icon");
        else res.set_header("Content-Type", "application/octet-stream");

        return res;
    });

    // Serve files in subdirectories (css/, js/)
    CROW_ROUTE(app, "/<string>/<string>")
    ([](const crow::request&, std::string subdir, std::string filename) {
        std::string path = "frontend/" + subdir + "/" + filename;
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return crow::response(404, "Not Found");

        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        crow::response res(content);

        if (filename.find(".css") != std::string::npos) res.set_header("Content-Type", "text/css; charset=utf-8");
        else if (filename.find(".js") != std::string::npos) res.set_header("Content-Type", "application/javascript; charset=utf-8");
        else res.set_header("Content-Type", "application/octet-stream");

        return res;
    });

    // Root redirects to init or dashboard
    CROW_ROUTE(app, "/"
    )([]() {
        if (!AppConfig::instance().initialized) {
            crow::response res;
            res.redirect("/init.html");
            return res;
        }
        crow::response res;
        res.redirect("/index.html");
        return res;
    });
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "==========================================\n";
    std::cout << "    Smart Parking 停车管理系统 v1.0\n";
    std::cout << "==========================================\n\n";

    crow::SimpleApp app;

    // Load config
    std::string configPath = findConfigPath();
    bool initialized = false;

    if (!configPath.empty()) {
        initialized = AppConfig::instance().load(configPath);
        if (initialized) {
            // Initialize database pool
            if (MySQLPool::instance().init(AppConfig::instance())) {
                std::cout << "[OK] 数据库连接成功\n";
            } else {
                std::cout << "[WARN] 数据库连接失败，请检查配置\n";
                initialized = false;
            }
        }
    }

    // Setup static file serving
    setupStaticFiles(app);

    // Init endpoint - always available
    CROW_ROUTE(app, "/api/init/status").methods("GET"_method)([]() {
        crow::json::wvalue res;
        res["initialized"] = AppConfig::instance().initialized;
        if (AppConfig::instance().initialized) {
            res["parking_name"] = AppConfig::instance().parking_name;
            res["server_port"] = AppConfig::instance().server_port;
        }
        return crow::response(res);
    });

    CROW_ROUTE(app, "/api/init/database").methods("POST"_method)([configPath](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return crow::response(400, "{\"error\":\"Invalid JSON\"}");

        AppConfig& cfg = AppConfig::instance();
        cfg.host = body["host"].s();
        cfg.port = body["port"].i();
        cfg.database = body["database"].s();
        cfg.user = body["user"].s();
        cfg.password = body["password"].s();
        cfg.parking_name = body["parking_name"].s();
        cfg.fee = body["fee"].d();
        cfg.capacity = body["capacity"].i();
        if (body.has("server_port")) cfg.server_port = body["server_port"].i();

        // Create database
        if (!DBInit::createDatabase(cfg))
            return crow::response(400, "{\"error\":\"创建数据库失败，请检查连接参数\"}");

        // Initialize connection pool
        if (!MySQLPool::instance().init(cfg))
            return crow::response(400, "{\"error\":\"数据库连接池初始化失败\"}");

        // Create tables
        if (!DBInit::createTables(cfg))
            return crow::response(400, "{\"error\":\"创建数据表失败\"}");

        // Save config
        cfg.save("config/db_config.json");

        return crow::response(200, "{\"message\":\"数据库初始化成功\"}");
    });

    // Register all API routes via polymorphic controller dispatch
    std::vector<std::unique_ptr<BaseController>> controllers;
    controllers.push_back(std::make_unique<AuthController>());
    controllers.push_back(std::make_unique<VehicleController>());
    controllers.push_back(std::make_unique<ParkingController>());
    controllers.push_back(std::make_unique<UserController>());
    controllers.push_back(std::make_unique<ReservationController>());
    controllers.push_back(std::make_unique<PlateController>());
    for (auto& ctrl : controllers) {
        ctrl->registerRoutes(app);
    }

    // CORS middleware - add to all responses
    app.loglevel(crow::LogLevel::Info);

    int port = AppConfig::instance().initialized ? AppConfig::instance().server_port : 8080;
    std::cout << "\n[OK] 服务启动于 http://localhost:" << port << "\n";
    std::cout << "[OK] 前端页面: http://localhost:" << port << "/index.html\n";
    std::cout << "[OK] 初始化向导: http://localhost:" << port << "/init.html\n\n";

    app.port(port).multithreaded().run();

    return 0;
}
