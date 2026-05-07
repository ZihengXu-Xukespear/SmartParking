#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <ctime>
#include "base_service.h"
#include "../model/user.h"
#include "../sha256.h"
#include "../permissions.h"

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
        time_t expiry = std::time(nullptr) + token_ttl_seconds_;
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        tokens_[token] = TokenInfo{out_user.id, out_user.role, expiry};
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
        auto it = tokens_.find(token);
        if (it == tokens_.end()) return false;
        if (std::time(nullptr) >= it->second.expiry) {
            tokens_.erase(it);
            return false;
        }
        return true;
    }

    int getUserId(const std::string& token) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        auto it = tokens_.find(token);
        if (it == tokens_.end()) return -1;
        if (std::time(nullptr) >= it->second.expiry) {
            tokens_.erase(it);
            return -1;
        }
        return it->second.userId;
    }

    std::string getUserRole(const std::string& token) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        auto it = tokens_.find(token);
        if (it == tokens_.end()) return "";
        if (std::time(nullptr) >= it->second.expiry) {
            tokens_.erase(it);
            return "";
        }
        return it->second.role;
    }

    void logout(const std::string& token) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        tokens_.erase(token);
    }

    // Remove expired tokens to reclaim memory
    void cleanupExpiredTokens() {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        time_t now = std::time(nullptr);
        for (auto it = tokens_.begin(); it != tokens_.end(); ) {
            if (now >= it->second.expiry)
                it = tokens_.erase(it);
            else
                ++it;
        }
    }

    // Check if the token holder has a specific permission
    bool hasPermission(const std::string& token, const std::string& permission) {
        std::string role = getUserRole(token);
        if (role.empty()) return false;
        const auto& perms = Permissions::getPermissionsForRole(role);
        return perms.find(permission) != perms.end();
    }

    // Get all permissions for the token holder
    std::vector<std::string> getUserPermissions(const std::string& token) {
        std::string role = getUserRole(token);
        const auto& perms = Permissions::getPermissionsForRole(role);
        return std::vector<std::string>(perms.begin(), perms.end());
    }

    static bool createDefaultAdmin() {
        auto conn = MySQLPool::instance().getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        // Check if root user already exists
        if (mysql_query(mysql, "SELECT COUNT(*) FROM USER WHERE role='root'") == 0) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row && std::stoi(row[0]) > 0) {
                    mysql_free_result(res);
                    return true;
                }
                mysql_free_result(res);
            }
        }

        std::string hashed = sha256::hash("admin123");
        std::string sql = "INSERT INTO USER (username,password,telephone,truename,role) VALUES "
            "('root','" + hashed + "','00000000000','系统管理员','root')";
        return mysql_query(mysql, sql.c_str()) == 0;
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

    struct TokenInfo {
        int userId;
        std::string role;
        time_t expiry;
    };
    std::unordered_map<std::string, TokenInfo> tokens_;
    std::mutex tokens_mutex_;
    std::mt19937 rng_;
    int token_ttl_seconds_ = 86400; // 24 hours
};
