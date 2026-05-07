#pragma once
#include "base_service.h"
#include "../model/user.h"
#include "../model/balance_record.h"

class BalanceService : public BaseService {
public:
    static BalanceService& instance() {
        static BalanceService inst;
        return inst;
    }

    // Get current balance for a user
    double getBalance(int userId) {
        auto conn = getConnection();
        if (!conn) return 0;
        std::string sql = "SELECT balance FROM USER WHERE id=" + std::to_string(userId);
        if (mysql_query(conn->get(), sql.c_str()) != 0) return 0;
        MYSQL_RES* res = mysql_store_result(conn->get());
        if (!res) return 0;
        MYSQL_ROW row = mysql_fetch_row(res);
        double bal = row && row[0] ? std::stod(row[0]) : 0;
        mysql_free_result(res);
        return bal;
    }

    // Admin recharge: add amount to user balance (atomic)
    bool recharge(int userId, double amount, const std::string& description, std::string& error) {
        if (amount <= 0) { error = "充值金额必须大于0"; return false; }
        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();

        std::string sql = "UPDATE USER SET balance = balance + " + std::to_string(amount) +
            " WHERE id=" + std::to_string(userId);
        if (executeQueryAffected(mysql, sql) <= 0) { error = "用户不存在"; return false; }

        double newBal = getBalance(userId);
        addRecord(userId, amount, "recharge", description, newBal);
        return true;
    }

    // Deduct balance — atomic with check (returns false if insufficient)
    bool deduct(int userId, double amount, const std::string& type, const std::string& description, std::string& error) {
        if (amount <= 0) { error = "扣款金额无效"; return false; }
        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();

        std::string sql = "UPDATE USER SET balance = balance - " + std::to_string(amount) +
            " WHERE id=" + std::to_string(userId) + " AND balance >= " + std::to_string(amount);
        if (executeQueryAffected(mysql, sql) <= 0) {
            double bal = getBalance(userId);
            error = "余额不足，当前余额: " + std::to_string(bal) + "元";
            return false;
        }

        double newBal = getBalance(userId);
        addRecord(userId, -amount, type, description, newBal);
        return true;
    }

    // Add refund (positive amount added back)
    bool refund(int userId, double amount, const std::string& type, const std::string& description) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        std::string sql = "UPDATE USER SET balance = balance + " + std::to_string(amount) +
            " WHERE id=" + std::to_string(userId);
        if (executeQueryAffected(mysql, sql) <= 0) return false;

        double newBal = getBalance(userId);
        addRecord(userId, amount, type, description, newBal);
        return true;
    }

    // Get transaction records
    std::vector<BalanceRecord> getTransactions(int userId, int limit = 50) {
        std::vector<BalanceRecord> records;
        auto conn = getConnection();
        if (!conn) return records;

        std::string sql = "SELECT id,user_id,amount,type,description,balance_after,created_at FROM BALANCE_RECORD";
        if (userId > 0) sql += " WHERE user_id=" + std::to_string(userId);
        sql += " ORDER BY created_at DESC LIMIT " + std::to_string(limit);

        if (mysql_query(conn->get(), sql.c_str()) != 0) return records;
        MYSQL_RES* res = mysql_store_result(conn->get());
        if (!res) return records;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            BalanceRecord r;
            r.id = std::stoi(row[0]);
            r.user_id = std::stoi(row[1]);
            r.amount = row[2] ? std::stod(row[2]) : 0;
            r.type = row[3] ? row[3] : "";
            r.description = row[4] ? row[4] : "";
            r.balance_after = row[5] ? std::stod(row[5]) : 0;
            r.created_at = row[6] ? row[6] : "";
            records.push_back(r);
        }
        mysql_free_result(res);
        return records;
    }

private:
    BalanceService() = default;

    void addRecord(int userId, double amount, const std::string& type,
                   const std::string& description, double balanceAfter) {
        auto conn = getConnection();
        if (!conn) return;
        MYSQL* mysql = conn->get();
        std::string sql = "INSERT INTO BALANCE_RECORD (user_id,amount,type,description,balance_after) VALUES (" +
            std::to_string(userId) + "," + std::to_string(amount) + ",'" +
            escape(mysql, type) + "','" + escape(mysql, description) + "'," +
            std::to_string(balanceAfter) + ")";
        mysql_query(mysql, sql.c_str());
    }
};
