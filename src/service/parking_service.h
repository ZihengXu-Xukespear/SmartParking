#pragma once
#include "base_service.h"
#include "../model/parking_lot.h"

class ParkingService : public BaseService {
public:
    static ParkingService& instance();
    ParkingLot getStatus();
    bool updateSettings(double fee, int total_count);
private:
    ParkingService() = default;
};
