#pragma once
#include <string>
#include <vector>
#include "base_service.h"
#include "../model/billing.h"

class BillingService : public BaseService {
public:
    static BillingService& instance() {
        static BillingService inst;
        return inst;
    }

    std::vector<BillingRule> getRules() {
        std::vector<BillingRule> rules;
        auto conn = getConnection();
        if (!conn) return rules;

        if (mysql_query(conn->get(), "SELECT id,rule_name,rule_type,free_minutes,hourly_rate,max_daily_fee,tier_config,description,is_active FROM BILLING_RULE ORDER BY id") != 0)
            return rules;
        MYSQL_RES* res = mysql_store_result(conn->get());
        if (!res) return rules;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            BillingRule r;
            r.id = std::stoi(row[0]);
            r.rule_name = row[1] ? row[1] : "";
            r.rule_type = row[2] ? row[2] : "";
            r.free_minutes = row[3] ? std::stoi(row[3]) : 30;
            r.hourly_rate = row[4] ? std::stod(row[4]) : 5.0;
            r.max_daily_fee = row[5] ? std::stod(row[5]) : 50.0;
            r.tier_config = row[6] ? row[6] : "";
            r.description = row[7] ? row[7] : "";
            r.is_active = row[8] ? std::stoi(row[8]) == 1 : true;
            rules.push_back(r);
        }
        mysql_free_result(res);
        return rules;
    }

    bool updateRule(int id, const BillingRule& rule) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        std::string sql = "UPDATE BILLING_RULE SET rule_name='" + escape(mysql, rule.rule_name) +
            "', rule_type='" + escape(mysql, rule.rule_type) +
            "', free_minutes=" + std::to_string(rule.free_minutes) +
            ", hourly_rate=" + std::to_string(rule.hourly_rate) +
            ", max_daily_fee=" + std::to_string(rule.max_daily_fee) +
            ", description='" + escape(mysql, rule.description) +
            "', is_active=" + std::to_string(rule.is_active ? 1 : 0) +
            " WHERE id=" + std::to_string(id);
        return executeQuery(mysql, sql);
    }

    std::vector<MonthlyPass> getMonthlyPasses() {
        std::vector<MonthlyPass> passes;
        auto conn = getConnection();
        if (!conn) return passes;

        if (mysql_query(conn->get(), "SELECT id,license_plate,pass_type,start_date,end_date,fee,is_active FROM MONTHLY_PASS ORDER BY id") != 0)
            return passes;
        MYSQL_RES* res = mysql_store_result(conn->get());
        if (!res) return passes;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            MonthlyPass p;
            p.id = std::stoi(row[0]);
            p.license_plate = row[1] ? row[1] : "";
            p.pass_type = row[2] ? row[2] : "";
            p.start_date = row[3] ? row[3] : "";
            p.end_date = row[4] ? row[4] : "";
            p.fee = row[5] ? std::stod(row[5]) : 0.0;
            p.is_active = row[6] ? std::stoi(row[6]) == 1 : true;
            passes.push_back(p);
        }
        mysql_free_result(res);
        return passes;
    }

    bool addMonthlyPass(const MonthlyPass& pass) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        std::string sql = "INSERT INTO MONTHLY_PASS (license_plate,pass_type,start_date,end_date,fee,is_active) VALUES ('" +
            escape(mysql, pass.license_plate) + "','" +
            escape(mysql, pass.pass_type) + "','" +
            escape(mysql, pass.start_date) + "','" +
            escape(mysql, pass.end_date) + "'," +
            std::to_string(pass.fee) + ",1)";
        return executeQuery(mysql, sql);
    }

    bool deleteMonthlyPass(int id) {
        auto conn = getConnection();
        if (!conn) return false;
        std::string sql = "DELETE FROM MONTHLY_PASS WHERE id=" + std::to_string(id);
        return executeQuery(conn->get(), sql);
    }

private:
    BillingService() = default;
};
