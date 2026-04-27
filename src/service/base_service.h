#pragma once
#include <string>
#include <memory>
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
        char* buf = new char[s.size() * 2 + 1];
        mysql_real_escape_string(mysql, buf, s.c_str(), (unsigned long)s.size());
        std::string result(buf);
        delete[] buf;
        return result;
    }

    bool executeQuery(MYSQL* mysql, const std::string& sql) {
        return mysql_query(mysql, sql.c_str()) == 0;
    }
};
