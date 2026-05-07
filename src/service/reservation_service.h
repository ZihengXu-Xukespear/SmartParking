#pragma once
#include "crud_service.h"
#include "../model/reservation.h"

class ReservationService : public CrudService<Reservation> {
public:
    static ReservationService& instance();
    bool create(const std::string& plate, const std::string& P_name, int userId, std::string& error);
    std::vector<Reservation> list();
    bool cancel(int id);
protected:
    Reservation mapRow(MYSQL_ROW row) override;
private:
    ReservationService() = default;
};
