#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include "crud_service.h"
#include "../model/car_record.h"
#include "../model/parking_lot.h"
#include "../config.h"

class VehicleService : public CrudService<CarRecord> {
public:
    static VehicleService& instance() {
        static VehicleService inst;
        return inst;
    }

    bool checkIn(const std::string& plate, const std::string& billing_type, std::string& error) {
        if (!validatePlate(plate)) { error = "车牌号格式不正确"; return false; }

        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();

        // Check if already inside
        std::string sql = "SELECT id FROM CAR_RECORD WHERE license_plate='" +
            escape(mysql, plate) + "' AND check_out_time IS NULL";
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "查询失败"; return false; }
        MYSQL_RES* res = mysql_store_result(mysql);
        if (res && mysql_num_rows(res) > 0) {
            mysql_free_result(res);
            error = "该车辆已在停车场内";
            return false;
        }
        if (res) mysql_free_result(res);

        // Check capacity
        const auto& cfg = AppConfig::instance();
        sql = "SELECT P_total_count, P_current_count, P_reserve_count FROM PARKING_LOT WHERE P_name='" +
            escape(mysql, cfg.parking_name) + "'";
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "查询停车场失败"; return false; }
        res = mysql_store_result(mysql);
        if (!res || mysql_num_rows(res) == 0) { error = "停车场不存在"; return false; }
        MYSQL_ROW row = mysql_fetch_row(res);
        int total = std::stoi(row[0]), current = std::stoi(row[1]), reserve = std::stoi(row[2]);
        mysql_free_result(res);

        if (current + reserve >= total) { error = "停车场已满"; return false; }

        // Insert record
        sql = "INSERT INTO CAR_RECORD (license_plate,check_in_time,location,billing_type) VALUES ('" +
            escape(mysql, plate) + "',NOW(),'" + escape(mysql, cfg.parking_name) + "','" +
            escape(mysql, billing_type) + "')";
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "插入记录失败"; return false; }

        // Increment current count
        sql = "UPDATE PARKING_LOT SET P_current_count = P_current_count + 1 WHERE P_name='" +
            escape(mysql, cfg.parking_name) + "'";
        mysql_query(mysql, sql.c_str());

        return true;
    }

    bool checkOut(const std::string& plate, double& fee, CarRecord& record, std::string& error) {
        auto conn = getConnection();
        if (!conn) { error = "数据库连接失败"; return false; }
        MYSQL* mysql = conn->get();

        // Find unchecked record
        std::string sql = "SELECT id,license_plate,check_in_time,billing_type FROM CAR_RECORD WHERE license_plate='" +
            escape(mysql, plate) + "' AND check_out_time IS NULL ORDER BY check_in_time DESC LIMIT 1";
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "查询失败"; return false; }
        MYSQL_RES* res = mysql_store_result(mysql);
        if (!res || mysql_num_rows(res) == 0) {
            if (res) mysql_free_result(res);
            error = "该车辆不在停车场内";
            return false;
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        int rec_id = std::stoi(row[0]);
        std::string billing_type = row[3] ? row[3] : "standard";
        std::string check_in = row[2] ? row[2] : "";
        mysql_free_result(res);

        // Calculate fee
        fee = calculateFee(mysql, plate, check_in, billing_type);

        // Update record
        sql = "UPDATE CAR_RECORD SET check_out_time=NOW(), fee=" + std::to_string(fee) +
            " WHERE id=" + std::to_string(rec_id);
        if (mysql_query(mysql, sql.c_str()) != 0) { error = "更新记录失败"; return false; }

        // Decrement current count
        const auto& cfg = AppConfig::instance();
        sql = "UPDATE PARKING_LOT SET P_current_count = GREATEST(P_current_count - 1, 0) WHERE P_name='" +
            escape(mysql, cfg.parking_name) + "'";
        mysql_query(mysql, sql.c_str());

        // Fetch updated record
        sql = "SELECT id,license_plate,check_in_time,check_out_time,fee,location,billing_type FROM CAR_RECORD WHERE id=" +
            std::to_string(rec_id);
        if (mysql_query(mysql, sql.c_str()) == 0) {
            res = mysql_store_result(mysql);
            if (res && (row = mysql_fetch_row(res))) {
                record.id = std::stoi(row[0]);
                record.license_plate = row[1] ? row[1] : "";
                record.check_in_time = row[2] ? row[2] : "";
                record.check_out_time = row[3] ? row[3] : "";
                record.fee = row[4] ? std::stod(row[4]) : 0.0;
                record.location = row[5] ? row[5] : "";
                record.billing_type = row[6] ? row[6] : "standard";
                mysql_free_result(res);
            }
        }

        return true;
    }

    std::vector<CarRecord> queryRecords(const std::string& plate, const std::string& start_date, const std::string& end_date) {
        auto conn = getConnection();
        if (!conn) return {};
        MYSQL* mysql = conn->get();

        std::string sql = "SELECT id,license_plate,check_in_time,check_out_time,fee,location,billing_type FROM CAR_RECORD WHERE 1=1";
        if (!plate.empty()) sql += " AND license_plate='" + escape(mysql, plate) + "'";
        if (!start_date.empty()) sql += " AND check_in_time >= '" + escape(mysql, start_date) + " 00:00:00'";
        if (!end_date.empty()) sql += " AND check_in_time <= '" + escape(mysql, end_date) + " 23:59:59'";
        sql += " ORDER BY check_in_time DESC LIMIT 500";

        return list(sql);
    }

    bool deleteRecord(int id) {
        return deleteById(id);
    }

protected:
    CarRecord mapRow(MYSQL_ROW row) override {
        CarRecord r;
        r.id = std::stoi(row[0]);
        r.license_plate = row[1] ? row[1] : "";
        r.check_in_time = row[2] ? row[2] : "";
        r.check_out_time = row[3] ? row[3] : "";
        r.fee = row[4] ? std::stod(row[4]) : 0.0;
        r.location = row[5] ? row[5] : "";
        r.billing_type = row[6] ? row[6] : "standard";
        return r;
    }

private:
    VehicleService() = default;

    double calculateFee(MYSQL* mysql, const std::string& plate, const std::string& check_in_time, const std::string& billing_type) {
        // Check monthly pass first
        std::string sql = "SELECT COUNT(*) FROM MONTHLY_PASS WHERE license_plate='" +
            escape(mysql, plate) + "' AND is_active=1 AND start_date <= CURDATE() AND end_date >= CURDATE()";
        if (mysql_query(mysql, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row && std::stoi(row[0]) > 0) {
                    mysql_free_result(res);
                    return 0.0;
                }
                mysql_free_result(res);
            }
        }

        double hourly_rate = AppConfig::instance().fee;
        int free_minutes = 30;
        double max_daily_fee = 50.0;
        std::string tier_config;

        sql = "SELECT free_minutes,hourly_rate,max_daily_fee,tier_config FROM BILLING_RULE WHERE rule_type='" +
            escape(mysql, billing_type) + "' AND is_active=1 LIMIT 1";
        if (mysql_query(mysql, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row) {
                    free_minutes = row[0] ? std::stoi(row[0]) : 30;
                    hourly_rate = row[1] ? std::stod(row[1]) : hourly_rate;
                    max_daily_fee = row[2] ? std::stod(row[2]) : 50.0;
                    tier_config = row[3] ? row[3] : "";
                }
                mysql_free_result(res);
            }
        }

        sql = "SELECT TIMESTAMPDIFF(MINUTE, '" + escape(mysql, check_in_time) + "', NOW())";
        int duration_min = 0;
        if (mysql_query(mysql, sql.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(mysql);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row && row[0]) duration_min = std::stoi(row[0]);
                mysql_free_result(res);
            }
        }

        if (duration_min <= free_minutes) return 0.0;

        double fee = 0.0;

        if (billing_type == "tiered" && !tier_config.empty()) {
            double remaining_hours = std::ceil((duration_min - free_minutes) / 60.0);
            size_t pos = 0;
            while (pos < tier_config.size()) {
                auto h_pos = tier_config.find("\"hours\":", pos);
                auto r_pos = tier_config.find("\"rate\":", pos);
                if (h_pos == std::string::npos || r_pos == std::string::npos) break;

                int tier_hours = std::stoi(tier_config.substr(h_pos + 8, tier_config.find("}", h_pos) - h_pos - 8));
                double tier_rate = std::stod(tier_config.substr(r_pos + 7));
                tier_rate = std::abs(tier_rate);

                double apply_hours = std::min(remaining_hours, (double)tier_hours);
                fee += apply_hours * tier_rate;
                remaining_hours -= apply_hours;
                pos = r_pos + 10;
                if (remaining_hours <= 0) break;
            }
            if (remaining_hours > 0) fee += remaining_hours * hourly_rate;
        } else {
            double hours = std::ceil((duration_min - free_minutes) / 60.0);
            fee = hours * hourly_rate;
        }

        if (max_daily_fee > 0 && fee > max_daily_fee) fee = max_daily_fee;

        fee = std::round(fee * 100.0) / 100.0;
        return fee;
    }

    bool validatePlate(const std::string& plate) {
        if (plate.size() < 9 || plate.size() > 10) return false;
        const std::string provinces = "京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤川青藏琼宁";
        bool found = false;
        for (size_t i = 0; i < provinces.size(); i += 3) {
            if (plate.size() >= 3 && provinces.substr(i, 3) == plate.substr(0, 3)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
        if (plate[3] < 'A' || plate[3] > 'Z') return false;
        return true;
    }
};
