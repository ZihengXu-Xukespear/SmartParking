#pragma once
#include <string>
#include <fstream>
#include "crow.h"

struct AppConfig {
    std::string host = "localhost";
    int port = 3306;
    std::string database = "smart_parking";
    std::string user = "root";
    std::string password;
    std::string parking_name = "智慧停车场";
    double fee = 5.00;
    int capacity = 100;
    int server_port = 8080;
    bool initialized = false;

    static AppConfig& instance() {
        static AppConfig inst;
        return inst;
    }

    bool load(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        try {
            auto j = crow::json::load(content);
            host = j["host"].s();
            port = j["port"].i();
            database = j["database"].s();
            user = j["user"].s();
            password = j["password"].s();
            parking_name = j["parking_name"].s();
            fee = j["fee"].d();
            capacity = j["capacity"].i();
            server_port = j["server_port"].i();
            initialized = true;
        } catch (...) {
            return false;
        }
        return true;
    }

    bool save(const std::string& path) {
        crow::json::wvalue j;
        j["host"] = host;
        j["port"] = port;
        j["database"] = database;
        j["user"] = user;
        j["password"] = password;
        j["parking_name"] = parking_name;
        j["fee"] = fee;
        j["capacity"] = capacity;
        j["server_port"] = server_port;
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump();
        initialized = true;
        return true;
    }
};
