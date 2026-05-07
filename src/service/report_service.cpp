#include "report_service.h"

ReportService& ReportService::instance() {
    static ReportService inst;
    return inst;
}

ReportService::RevenueSummary ReportService::getSummary() {
    RevenueSummary s;
    auto conn = getConnection();
    if (!conn) return s;
    MYSQL* mysql = conn->get();

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

std::vector<std::pair<std::string, double>> ReportService::getDailyRevenue(int days) {
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
