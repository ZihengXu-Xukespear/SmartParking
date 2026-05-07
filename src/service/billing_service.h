#pragma once
#include "base_service.h"
#include "../model/billing.h"

class BillingService : public BaseService {
public:
    static BillingService& instance();
    std::vector<BillingRule> getRules();
    bool updateRule(int id, const BillingRule& rule);
    std::vector<MonthlyPass> getMonthlyPasses();
    bool addMonthlyPass(const MonthlyPass& pass);
    bool updateMonthlyPass(int id, const MonthlyPass& pass);
    bool deleteMonthlyPass(int id);
private:
    BillingService() = default;
};
