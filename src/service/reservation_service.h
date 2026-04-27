#pragma once
#include <string>
#include <vector>
#include "crud_service.h"
#include "../model/reservation.h"
#include "../config.h"

class ReservationService : public CrudService<Reservation> {
public:
    static ReservationService& instance() {
        static ReservationService inst;
        return inst;
    }

    bool create(const std::string& plate, const std::string& P_name, std::string& error) {
        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();

        // Check if already reserved
        std::string sql = "SELECT id FROM RESERVATION WHERE license_plate='" + escape(mysql, plate) + "'";
        if (mysql_query(mysql, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (res && mysql_num_rows(res) > 0) {
                mysql_free_result(res);
                error = "该车牌已有预约";
                return false;
            }
            if (res) mysql_free_result(res);
        }

        // Check capacity
        sql = "SELECT P_total_count, P_current_count, P_reserve_count FROM PARKING_LOT WHERE P_name='" +
            escape(mysql, P_name) + "'";
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "查询停车场失败"; return false; }
        MYSQL_RES* res = mysql_store_result(mysql);
        if (!res || mysql_num_rows(res) == 0) { error = "停车场不存在"; return false; }
        MYSQL_ROW row = mysql_fetch_row(res);
        int total = std::stoi(row[0]), current = std::stoi(row[1]), reserve = std::stoi(row[2]);
        mysql_free_result(res);

        if (current + reserve >= total) { error = "停车场已满，无法预约"; return false; }

        // Insert reservation
        sql = "INSERT INTO RESERVATION (license_plate, P_name) VALUES ('" +
            escape(mysql, plate) + "','" + escape(mysql, P_name) + "')";
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "预约失败"; return false; }

        return true;
    }

    std::vector<Reservation> list() {
        return CrudService<Reservation>::list("SELECT id,license_plate,P_name,created_at FROM RESERVATION ORDER BY created_at DESC");
    }

    bool cancel(int id) {
        return deleteById(id);
    }

protected:
    Reservation mapRow(MYSQL_ROW row) override {
        Reservation r;
        r.id = std::stoi(row[0]);
        r.license_plate = row[1] ? row[1] : "";
        r.P_name = row[2] ? row[2] : "";
        r.created_at = row[3] ? row[3] : "";
        return r;
    }

private:
    ReservationService() = default;
};
