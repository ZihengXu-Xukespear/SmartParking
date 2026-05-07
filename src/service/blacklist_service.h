#pragma once
#include "crud_service.h"
#include "../model/blacklist.h"

class BlacklistService : public CrudService<BlacklistEntry> {
public:
    static BlacklistService& instance() {
        static BlacklistService inst;
        return inst;
    }

    std::vector<BlacklistEntry> getAll() {
        return list("SELECT id,license_plate,reason,created_at FROM VEHICLE_BLACKLIST ORDER BY created_at DESC");
    }

    bool isBlacklisted(const std::string& plate) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();
        std::string sql = "SELECT COUNT(*) FROM VEHICLE_BLACKLIST WHERE license_plate='" +
            escape(mysql, plate) + "'";
        if (mysql_query(mysql, sql.c_str()) != 0) return false;
        MYSQL_RES* res = mysql_store_result(mysql);
        if (!res) return false;
        MYSQL_ROW row = mysql_fetch_row(res);
        bool blocked = row && std::stoi(row[0]) > 0;
        mysql_free_result(res);
        return blocked;
    }

    bool add(const std::string& plate, const std::string& reason, std::string& error) {
        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();
        std::string sql = "INSERT IGNORE INTO VEHICLE_BLACKLIST (license_plate,reason) VALUES ('" +
            escape(mysql, plate) + "','" + escape(mysql, reason) + "')";
        if (!executeQuery(mysql, sql)) { error = "添加失败"; return false; }
        return true;
    }

    bool remove(int id) {
        return deleteById(id);
    }

protected:
    BlacklistEntry mapRow(MYSQL_ROW row) override {
        BlacklistEntry e;
        e.id = std::stoi(row[0]);
        e.license_plate = row[1] ? row[1] : "";
        e.reason = row[2] ? row[2] : "";
        e.created_at = row[3] ? row[3] : "";
        return e;
    }

private:
    BlacklistService() = default;
};
