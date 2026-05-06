#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <shared_mutex>
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
    int notice_expire_minutes = 30;
    std::string notice = "欢迎使用智慧停车场管理系统！\n请遵守停车场管理规定，文明停车。";
    bool initialized = false;
    std::string config_dir = "config";
    std::string config_file = "config/db_config.json";

    static AppConfig& instance() {
        static AppConfig inst;
        return inst;
    }

    bool load(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        auto j = crow::json::load(content);
        if (!j) return false;
        std::lock_guard<std::shared_mutex> lock(mutex_);
        host = j["host"].s();
        port = j["port"].i();
        database = j["database"].s();
        user = j["user"].s();
        password = j["password"].s();
        parking_name = j["parking_name"].s();
        fee = j["fee"].d();
        capacity = j["capacity"].i();
        server_port = j["server_port"].i();
        if (j.has("notice")) notice = j["notice"].s();
        if (j.has("notice_expire_minutes")) notice_expire_minutes = j["notice_expire_minutes"].i();
        initialized = true;
        return true;
    }

    bool save(const std::string& path) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
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
        j["notice_expire_minutes"] = notice_expire_minutes;
        j["notice"] = notice;
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << j.dump();
        initialized = true;
        return true;
    }

    // Thread-safe read lock for concurrent reads
    std::shared_mutex mutex_;
};
