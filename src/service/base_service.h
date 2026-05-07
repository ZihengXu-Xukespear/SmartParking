#pragma once
#include <string>
#include <memory>
#include <vector>
#include "../database/mysql_pool.h"

class BaseService {
public:
    virtual ~BaseService() = default;

    BaseService(const BaseService&) = delete;
    BaseService& operator=(const BaseService&) = delete;

protected:
    BaseService() = default;

    std::shared_ptr<MySQLPool::ConnGuard> getConnection() {
        return MySQLPool::instance().getConnection();
    }

    static std::string escape(MYSQL* mysql, const std::string& s) {
        std::vector<char> buf(s.size() * 2 + 1);
        size_t len = mysql_real_escape_string(mysql, buf.data(), s.c_str(), (unsigned long)s.size());
        return std::string(buf.data(), len);
    }

    // Quote and escape a string value for SQL
    static std::string quote(MYSQL* mysql, const std::string& s) {
        return "'" + escape(mysql, s) + "'";
    }

    bool executeQuery(MYSQL* mysql, const std::string& sql) {
        return mysql_query(mysql, sql.c_str()) == 0;
    }

    int executeQueryAffected(MYSQL* mysql, const std::string& sql) {
        if (mysql_query(mysql, sql.c_str()) != 0) return -1;
        return (int)mysql_affected_rows(mysql);
    }
};
