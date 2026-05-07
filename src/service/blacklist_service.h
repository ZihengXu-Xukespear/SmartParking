#pragma once
#include "crud_service.h"
#include "../model/blacklist.h"

class BlacklistService : public CrudService<BlacklistEntry> {
public:
    static BlacklistService& instance();
    std::vector<BlacklistEntry> getAll();
    bool isBlacklisted(const std::string& plate);
    bool add(const std::string& plate, const std::string& reason, std::string& error);
    bool remove(int id);
protected:
    BlacklistEntry mapRow(MYSQL_ROW row) override;
private:
    BlacklistService() = default;
};
