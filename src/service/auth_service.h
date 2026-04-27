#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include "base_service.h"
#include "../model/user.h"
#include "../sha256.h"

class AuthService : public BaseService {
public:
    static AuthService& instance() {
        static AuthService inst;
        return inst;
    }

    std::string login(const std::string& username, const std::string& password, User& out_user) {
        auto conn = getConnection();
        if (!conn) return "";

        std::string hashed = sha256::hash(password);
        std::string sql = "SELECT id,username,password,telephone,truename,role,created_at FROM USER WHERE username='" +
            escape(conn->get(), username) + "' AND password='" + escape(conn->get(), hashed) + "'";

        if (mysql_query(conn->get(), sql.c_str()) != 0) return "";
        MYSQL_RES* res = mysql_store_result(conn->get());
        if (!res || mysql_num_rows(res) == 0) {
            if (res) mysql_free_result(res);
            return "";
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        out_user.id = std::stoi(row[0]);
        out_user.username = row[1] ? row[1] : "";
        out_user.password = row[2] ? row[2] : "";
        out_user.telephone = row[3] ? row[3] : "";
        out_user.truename = row[4] ? row[4] : "";
        out_user.role = row[5] ? row[5] : "user";
        out_user.created_at = row[6] ? row[6] : "";
        mysql_free_result(res);

        std::string token = generateToken();
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        tokens_[token] = out_user.id;
        return token;
    }

    bool registerUser(const User& user) {
        auto conn = getConnection();
        if (!conn) return false;

        std::string hashed = sha256::hash(user.password);
        std::string sql = "INSERT INTO USER (username,password,telephone,truename,role) VALUES ('" +
            escape(conn->get(), user.username) + "','" +
            escape(conn->get(), hashed) + "','" +
            escape(conn->get(), user.telephone) + "','" +
            escape(conn->get(), user.truename) + "','" +
            escape(conn->get(), user.role) + "')";

        return mysql_query(conn->get(), sql.c_str()) == 0;
    }

    bool validateToken(const std::string& token) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        return tokens_.find(token) != tokens_.end();
    }

    int getUserId(const std::string& token) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        auto it = tokens_.find(token);
        return it != tokens_.end() ? it->second : -1;
    }

    void logout(const std::string& token) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        tokens_.erase(token);
    }

private:
    AuthService() {
        std::random_device rd;
        rng_.seed(rd());
    }

    std::string generateToken() {
        static const char chars[] = "0123456789abcdef";
        std::uniform_int_distribution<int> dist(0, 15);
        std::string token;
        for (int i = 0; i < 64; i++) token += chars[dist(rng_)];
        return token;
    }

    std::unordered_map<std::string, int> tokens_;
    std::mutex tokens_mutex_;
    std::mt19937 rng_;
};
