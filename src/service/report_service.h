#pragma once
#include "base_service.h"
#include <vector>
#include <string>

class ReportService : public BaseService {
public:
    static ReportService& instance();

    struct RevenueSummary {
        double today_income = 0;
        double month_income = 0;
        double total_income = 0;
        double parking_fees = 0;
        double pass_sales = 0;
        double reservation_prepaid = 0;
    };

    RevenueSummary getSummary();
    std::vector<std::pair<std::string, double>> getDailyRevenue(int days = 30);
private:
    ReportService() = default;
};
