#pragma once
#include <string>
#include <vector>
#include "crud_service.h"
#include "../model/user.h"
#include "../sha256.h"

class UserService : public CrudService<User> {
public:
    static UserService& instance() {
        static UserService inst;
        return inst;
    }

    std::vector<User> listUsers() {
        return list("SELECT id,username,telephone,truename,role,created_at FROM USER ORDER BY id");
    }

    bool addUser(const User& user) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        std::string hashed = sha256::hash(user.password);
        std::string sql = "INSERT INTO USER (username,password,telephone,truename,role) VALUES ('" +
            escape(mysql, user.username) + "','" +
            escape(mysql, hashed) + "','" +
            escape(mysql, user.telephone) + "','" +
            escape(mysql, user.truename) + "','" +
            escape(mysql, user.role) + "')";
        return executeQuery(mysql, sql);
    }

    bool updateUser(int id, const std::string& telephone, const std::string& truename, const std::string& role) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        std::string sql = "UPDATE USER SET telephone='" + escape(mysql, telephone) +
            "', truename='" + escape(mysql, truename) +
            "', role='" + escape(mysql, role) +
            "' WHERE id=" + std::to_string(id);
        return executeQuery(mysql, sql);
    }

    bool updateUserPassword(int id, const std::string& password) {
        auto conn = getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        std::string hashed = sha256::hash(password);
        std::string sql = "UPDATE USER SET password='" + escape(mysql, hashed) +
            "' WHERE id=" + std::to_string(id);
        return executeQuery(mysql, sql);
    }

    bool deleteUser(int id) {
        return deleteById(id);
    }

protected:
    User mapRow(MYSQL_ROW row) override {
        User u;
        u.id = std::stoi(row[0]);
        u.username = row[1] ? row[1] : "";
        u.telephone = row[2] ? row[2] : "";
        u.truename = row[3] ? row[3] : "";
        u.role = row[4] ? row[4] : "user";
        u.created_at = row[5] ? row[5] : "";
        return u;
    }

private:
    UserService() = default;
};
