#pragma once
#include <string>
#include <vector>
#include "crud_service.h"
#include "../model/reservation.h"
#include "../config.h"
#include "balance_service.h"

class ReservationService : public CrudService<Reservation> {
public:
    static ReservationService& instance() {
        static ReservationService inst;
        return inst;
    }

    bool create(const std::string& plate, const std::string& P_name, int userId, std::string& error) {
        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();
        Transaction tx(mysql);

        // Check if already reserved
        std::string sql = "SELECT id FROM RESERVATION WHERE license_plate=" + quote(mysql, plate) + " AND status='active'";
        if (mysql_query(mysql, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (res && mysql_num_rows(res) > 0) {
                mysql_free_result(res);
                error = "该车牌已有预约";
                return false; // tx rollback
            }
            if (res) mysql_free_result(res);
        }

        // Clean up expired/completed reservation for this plate to allow new reservation
        // (the UNIQUE index prevents duplicate license_plate rows)
        sql = "DELETE FROM RESERVATION WHERE license_plate=" + quote(mysql, plate) + " AND status != 'active'";
        mysql_query(mysql, sql.c_str());

        // Check capacity and get fee
        sql = "SELECT P_total_count, P_current_count, P_reserve_count, P_fee FROM PARKING_LOT WHERE P_name=" +
            quote(mysql, P_name);
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "查询停车场失败"; return false; }
        MYSQL_RES* res = mysql_store_result(mysql);
        if (!res) { error = "停车场不存在"; return false; }
        if (mysql_num_rows(res) == 0) { mysql_free_result(res); error = "停车场不存在"; return false; }
        MYSQL_ROW row = mysql_fetch_row(res);
        int total = std::stoi(row[0]), current = std::stoi(row[1]), reserve = std::stoi(row[2]);
        double prepaidFee = row[3] ? std::stod(row[3]) : 5.00;
        mysql_free_result(res);

        if (current + reserve >= total) { error = "停车场已满，无法预约"; return false; }

        // Check if plate has an active monthly pass — free reservation for pass holders
        bool hasPass = false;
        sql = "SELECT COUNT(*) FROM MONTHLY_PASS WHERE license_plate=" + quote(mysql, plate) +
            " AND is_active=1 AND start_date <= CURDATE() AND end_date >= CURDATE()";
        if (mysql_query(mysql, sql.c_str()) == 0) {
            MYSQL_RES* pr = mysql_store_result(mysql);
            if (pr) { MYSQL_ROW prow = mysql_fetch_row(pr); hasPass = (prow && std::stoi(prow[0]) > 0); mysql_free_result(pr); }
        }

        if (hasPass) {
            prepaidFee = 0;
        } else {
            std::string deductErr;
            if (!BalanceService::instance().deduct(userId, prepaidFee, "reservation",
                "预约预付 " + plate + " (首小时费用)", deductErr)) {
                error = deductErr;
                return false;
            }
        }

        // Insert reservation
        sql = "INSERT INTO RESERVATION (license_plate, P_name, prepaid, status) VALUES (" +
            quote(mysql, plate) + "," + quote(mysql, P_name) + "," +
            std::to_string(prepaidFee) + ",'active')";
        if (mysql_query(mysql, sql.c_str()) != 0) {
            if (!hasPass) BalanceService::instance().refund(userId, prepaidFee, "refund", "预约创建失败退款 " + plate);
            error = "预约失败";
            return false;
        }

        if (!tx.commit()) { error = "事务提交失败"; return false; }
        return true;
    }

    std::vector<Reservation> list() {
        return CrudService<Reservation>::list("SELECT id,license_plate,P_name,prepaid,status,created_at FROM RESERVATION WHERE status='active' ORDER BY created_at DESC");
    }

    bool cancel(int id) {
        // Hard delete — trigger after_reservation_delete handles reserve count, no refund
        return deleteById(id);
    }

protected:
    Reservation mapRow(MYSQL_ROW row) override {
        Reservation r;
        r.id = std::stoi(row[0]);
        r.license_plate = row[1] ? row[1] : "";
        r.P_name = row[2] ? row[2] : "";
        r.prepaid = row[3] ? std::stod(row[3]) : 0;
        r.status = row[4] ? row[4] : "";
        r.created_at = row[5] ? row[5] : "";
        return r;
    }

private:
    ReservationService() = default;
};
