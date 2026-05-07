#pragma once
#include "base_service.h"
#include "../model/user.h"
#include "../model/balance_record.h"

class BalanceService : public BaseService {
public:
    static BalanceService& instance();
    double getBalance(int userId);
    bool recharge(int userId, double amount, const std::string& description, std::string& error);
    bool deduct(int userId, double amount, const std::string& type, const std::string& description, std::string& error);
    bool refund(int userId, double amount, const std::string& type, const std::string& description);
    std::vector<BalanceRecord> getTransactions(int userId, int limit = 50);
private:
    BalanceService() = default;
    void addRecord(int userId, double amount, const std::string& type,
                   const std::string& description, double balanceAfter);
};
