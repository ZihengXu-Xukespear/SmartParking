#pragma once
#include "base_service.h"
#include <vector>
#include <string>

class ReportService : public BaseService {
public:
    static ReportService& instance() {
        static ReportService inst;
        return inst;
    }

    struct RevenueSummary {
        double today_income = 0;
        double month_income = 0;
        double total_income = 0;
        double parking_fees = 0;
        double pass_sales = 0;
        double reservation_prepaid = 0;
    };

    RevenueSummary getSummary() {
        RevenueSummary s;
        auto conn = getConnection();
        if (!conn) return s;
        MYSQL* mysql = conn->get();

        // Helper lambda: run a filtered query with GROUP BY type, parse into summary fields
        auto queryPeriod = [&](const std::string& where_clause, double& total) {
            std::string sql = "SELECT type, COALESCE(SUM(ABS(amount)),0) FROM BALANCE_RECORD "
                "WHERE amount<0 " + where_clause + " GROUP BY type";
            if (mysql_query(mysql, sql.c_str()) != 0) return;
            MYSQL_RES* res = mysql_store_result(mysql);
            if (!res) return;
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                std::string type = row[0] ? row[0] : "";
                double val = row[1] ? std::stod(row[1]) : 0;
                total += val;
                if (type == "checkout") s.parking_fees += val;
                else if (type == "purchase") s.pass_sales += val;
                else if (type == "reservation") s.reservation_prepaid += val;
            }
            mysql_free_result(res);
        };

        queryPeriod("AND DATE(created_at)=CURDATE()", s.today_income);
        queryPeriod("AND YEAR(created_at)=YEAR(CURDATE()) AND MONTH(created_at)=MONTH(CURDATE())", s.month_income);
        queryPeriod("", s.total_income);

        return s;
    }

    // Daily revenue for last 30 days (for chart)
    std::vector<std::pair<std::string, double>> getDailyRevenue(int days = 30) {
        std::vector<std::pair<std::string, double>> data;
        auto conn = getConnection();
        if (!conn) return data;
        MYSQL* mysql = conn->get();

        std::string sql = "SELECT DATE(created_at) as d, COALESCE(SUM(ABS(amount)),0) FROM BALANCE_RECORD "
            "WHERE amount<0 AND created_at >= DATE_SUB(CURDATE(), INTERVAL " + std::to_string(days) + " DAY) "
            "GROUP BY DATE(created_at) ORDER BY d";
        if (mysql_query(mysql, sql.c_str()) != 0) return data;
        MYSQL_RES* res = mysql_store_result(mysql);
        if (!res) return data;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            data.push_back({row[0] ? row[0] : "", row[1] ? std::stod(row[1]) : 0});
        }
        mysql_free_result(res);
        return data;
    }

private:
    ReportService() = default;
};
