#pragma once
#include <mysql.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <chrono>
#include "../config.h"

// RAII transaction guard — disables autocommit, rolls back if not committed
class Transaction {
public:
    explicit Transaction(MYSQL* mysql) : mysql_(mysql), committed_(false) {
        if (mysql_) mysql_autocommit(mysql_, 0);
    }
    ~Transaction() {
        if (mysql_ && !committed_) {
            mysql_rollback(mysql_);
            mysql_autocommit(mysql_, 1);
        }
    }
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&& other) noexcept : mysql_(other.mysql_), committed_(other.committed_) {
        other.mysql_ = nullptr;
    }
    bool commit() {
        if (!mysql_) return false;
        committed_ = true;
        bool ok = mysql_commit(mysql_) == 0;
        mysql_autocommit(mysql_, 1);
        return ok;
    }
private:
    MYSQL* mysql_;
    bool committed_;
};

class MySQLPool {
public:
    static MySQLPool& instance() {
        static MySQLPool inst;
        return inst;
    }

    bool init(const AppConfig& cfg, int pool_size = 5) {
        std::lock_guard<std::mutex> lock(mutex_);
        mysql_library_init(0, nullptr, nullptr);
        for (int i = 0; i < pool_size; i++) {
            MYSQL* conn = mysql_init(nullptr);
            if (!conn) return false;
            mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
            mysql_options(conn, MYSQL_INIT_COMMAND, "SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci");
            if (!mysql_real_connect(conn, cfg.host.c_str(), cfg.user.c_str(),
                cfg.password.c_str(), cfg.database.c_str(), cfg.port, nullptr, 0)) {
                last_error_ = mysql_error(conn);
                mysql_close(conn);
                return false;
            }
            pool_.push(conn);
        }
        return true;
    }

    class ConnGuard {
    public:
        ConnGuard(MYSQL* conn, MySQLPool* pool) : conn_(conn), pool_(pool) {}
        ~ConnGuard() { if (conn_) pool_->returnConn(conn_); }
        MYSQL* get() { return conn_; }
        MYSQL* operator->() { return conn_; }
        ConnGuard(const ConnGuard&) = delete;
        ConnGuard& operator=(const ConnGuard&) = delete;
    private:
        MYSQL* conn_;
        MySQLPool* pool_;
    };

    std::shared_ptr<ConnGuard> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (pool_.empty()) {
            if (cv_.wait_for(lock, std::chrono::seconds(5)) == std::cv_status::timeout)
                return nullptr;
        }
        MYSQL* conn = pool_.front();
        pool_.pop();
        // Check connection is alive
        if (mysql_ping(conn) != 0) {
            const auto& cfg = AppConfig::instance();
            mysql_close(conn);
            conn = mysql_init(nullptr);
            mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
            if (!mysql_real_connect(conn, cfg.host.c_str(), cfg.user.c_str(),
                cfg.password.c_str(), cfg.database.c_str(), cfg.port, nullptr, 0)) {
                return nullptr;
            }
        }
        return std::make_shared<ConnGuard>(conn, this);
    }

    const std::string& lastError() const { return last_error_; }

    ~MySQLPool() {
        while (!pool_.empty()) {
            mysql_close(pool_.front());
            pool_.pop();
        }
        mysql_library_end();
    }

private:
    MySQLPool() = default;
    void returnConn(MYSQL* conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(conn);
        cv_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<MYSQL*> pool_;
    std::string last_error_;
};
