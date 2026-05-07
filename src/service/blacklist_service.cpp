#include "blacklist_service.h"

BlacklistService& BlacklistService::instance() {
    static BlacklistService inst;
    return inst;
}

std::vector<BlacklistEntry> BlacklistService::getAll() {
    return list("SELECT id,license_plate,reason,created_at FROM VEHICLE_BLACKLIST ORDER BY created_at DESC");
}

bool BlacklistService::isBlacklisted(const std::string& plate) {
    auto conn = getConnection();
    if (!conn) return false;
    MYSQL* mysql = conn->get();
    std::string sql = "SELECT COUNT(*) FROM VEHICLE_BLACKLIST WHERE license_plate=" + quote(mysql, plate);
    if (mysql_query(mysql, sql.c_str()) != 0) return false;
    MYSQL_RES* res = mysql_store_result(mysql);
    if (!res) return false;
    MYSQL_ROW row = mysql_fetch_row(res);
    bool blocked = row && std::stoi(row[0]) > 0;
    mysql_free_result(res);
    return blocked;
}

bool BlacklistService::add(const std::string& plate, const std::string& reason, std::string& error) {
    auto conn = getConnection();
    if (!conn) { error = "数据库连接失败"; return false; }
    MYSQL* mysql = conn->get();
    std::string sql = "INSERT IGNORE INTO VEHICLE_BLACKLIST (license_plate,reason) VALUES (" +
        quote(mysql, plate) + "," + quote(mysql, reason) + ")";
    if (!executeQuery(mysql, sql)) { error = "添加失败"; return false; }
    return true;
}

bool BlacklistService::remove(int id) {
    return deleteById(id);
}

BlacklistEntry BlacklistService::mapRow(MYSQL_ROW row) {
    BlacklistEntry e;
    e.id = std::stoi(row[0]);
    e.license_plate = row[1] ? row[1] : "";
    e.reason = row[2] ? row[2] : "";
    e.created_at = row[3] ? row[3] : "";
    return e;
}
