#pragma once
#include <string>
#include "mysql_pool.h"
#include "../config.h"

class DBInit {
public:
    static bool createDatabase(const AppConfig& cfg) {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) return false;
        mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        mysql_options(conn, MYSQL_INIT_COMMAND, "SET NAMES utf8mb4");
        if (!mysql_real_connect(conn, cfg.host.c_str(), cfg.user.c_str(),
            cfg.password.c_str(), nullptr, cfg.port, nullptr, 0)) {
            mysql_close(conn);
            return false;
        }
        std::string sql = "CREATE DATABASE IF NOT EXISTS `" + cfg.database +
            "` DEFAULT CHARACTER SET utf8mb4 DEFAULT COLLATE utf8mb4_unicode_ci";
        bool ok = mysql_query(conn, sql.c_str()) == 0;
        mysql_close(conn);
        return ok;
    }

    static bool createTables(const AppConfig& cfg) {
        auto conn = MySQLPool::instance().getConnection();
        if (!conn) return false;
        MYSQL* mysql = conn->get();

        const char* tables[] = {
            "CREATE TABLE IF NOT EXISTS USER ("
            "  id INT PRIMARY KEY AUTO_INCREMENT,"
            "  username VARCHAR(255) UNIQUE NOT NULL,"
            "  password VARCHAR(64) NOT NULL,"
            "  telephone VARCHAR(11) NOT NULL,"
            "  truename VARCHAR(255) NOT NULL,"
            "  role VARCHAR(20) DEFAULT 'user',"
            "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4",

            "CREATE TABLE IF NOT EXISTS PARKING_LOT ("
            "  P_id INT PRIMARY KEY AUTO_INCREMENT,"
            "  P_name VARCHAR(255) UNIQUE NOT NULL,"
            "  P_total_count INT NOT NULL DEFAULT 0,"
            "  P_current_count INT NOT NULL DEFAULT 0,"
            "  P_reserve_count INT NOT NULL DEFAULT 0,"
            "  P_fee DECIMAL(10,2) NOT NULL DEFAULT 5.00"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4",

            "CREATE TABLE IF NOT EXISTS CAR_RECORD ("
            "  id INT PRIMARY KEY AUTO_INCREMENT,"
            "  license_plate VARCHAR(20) NOT NULL,"
            "  check_in_time DATETIME NOT NULL,"
            "  check_out_time DATETIME DEFAULT NULL,"
            "  fee DECIMAL(10,2) DEFAULT NULL,"
            "  location VARCHAR(255) NOT NULL,"
            "  billing_type VARCHAR(20) DEFAULT 'standard',"
            "  INDEX idx_plate (license_plate),"
            "  INDEX idx_checkin (check_in_time)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4",

            "CREATE TABLE IF NOT EXISTS RESERVATION ("
            "  id INT PRIMARY KEY AUTO_INCREMENT,"
            "  license_plate VARCHAR(20) NOT NULL,"
            "  P_name VARCHAR(255) NOT NULL,"
            "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "  UNIQUE INDEX idx_plate (license_plate)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4",

            "CREATE TABLE IF NOT EXISTS BILLING_RULE ("
            "  id INT PRIMARY KEY AUTO_INCREMENT,"
            "  rule_name VARCHAR(100) NOT NULL,"
            "  rule_type VARCHAR(20) NOT NULL,"
            "  free_minutes INT DEFAULT 30,"
            "  hourly_rate DECIMAL(10,2) DEFAULT 5.00,"
            "  max_daily_fee DECIMAL(10,2) DEFAULT NULL,"
            "  tier_config TEXT DEFAULT NULL,"
            "  description TEXT DEFAULT NULL,"
            "  is_active TINYINT DEFAULT 1"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4",

            "CREATE TABLE IF NOT EXISTS MONTHLY_PASS ("
            "  id INT PRIMARY KEY AUTO_INCREMENT,"
            "  license_plate VARCHAR(20) NOT NULL,"
            "  pass_type VARCHAR(50) NOT NULL,"
            "  start_date DATE NOT NULL,"
            "  end_date DATE NOT NULL,"
            "  fee DECIMAL(10,2) NOT NULL,"
            "  is_active TINYINT DEFAULT 1,"
            "  INDEX idx_plate (license_plate)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
        };

        for (const char* sql : tables) {
            if (mysql_query(mysql, sql) != 0) return false;
        }

        // Insert default billing rules
        mysql_query(mysql,
            "INSERT IGNORE INTO BILLING_RULE (rule_name,rule_type,free_minutes,hourly_rate,max_daily_fee,description,is_active) VALUES "
            "('标准计费','standard',30,5.00,50.00,'30分钟内免费，之后每小时5元，每日封顶50元',1),"
            "('阶梯计费','tiered',30,5.00,60.00,'前2小时每小时5元，2-4小时每小时3元，4小时以上每小时2元',0),"
            "('会员计费','member',60,3.00,30.00,'会员享受60分钟免费，之后每小时3元，每日封顶30元',0),"
            "('特殊车辆','special',1440,0.00,0.00,'军车、警车、消防车等特殊车辆免费',0)"
        );

        // Insert default parking lot
        std::string parking_sql = "INSERT IGNORE INTO PARKING_LOT (P_name,P_total_count,P_current_count,P_reserve_count,P_fee) VALUES ('" +
            cfg.parking_name + "'," + std::to_string(cfg.capacity) + ",0,0," + std::to_string(cfg.fee) + ")";
        mysql_query(mysql, parking_sql.c_str());

        // Create triggers
        mysql_query(mysql, "DROP TRIGGER IF EXISTS after_reservation_insert");
        mysql_query(mysql, "DROP TRIGGER IF EXISTS after_reservation_delete");

        if (mysql_query(mysql,
            "CREATE TRIGGER after_reservation_insert "
            "AFTER INSERT ON RESERVATION FOR EACH ROW "
            "BEGIN UPDATE PARKING_LOT SET P_reserve_count = P_reserve_count + 1 WHERE P_name = NEW.P_name; END"
        ) != 0) return false;

        if (mysql_query(mysql,
            "CREATE TRIGGER after_reservation_delete "
            "AFTER DELETE ON RESERVATION FOR EACH ROW "
            "BEGIN UPDATE PARKING_LOT SET P_reserve_count = GREATEST(P_reserve_count - 1, 0) WHERE P_name = OLD.P_name; END"
        ) != 0) return false;

        // Enable event scheduler and create cleanup event
        mysql_query(mysql, "SET GLOBAL event_scheduler = ON");
        mysql_query(mysql, "DROP EVENT IF EXISTS clean_expired_reservations");
        if (mysql_query(mysql,
            "CREATE EVENT clean_expired_reservations ON SCHEDULE EVERY 1 MINUTE DO "
            "BEGIN DELETE FROM RESERVATION WHERE created_at < DATE_SUB(NOW(), INTERVAL 30 MINUTE); END"
        ) != 0) return false;

        return true;
    }
};
