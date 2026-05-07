#pragma once
#include <string>
#include "base_service.h"

class PlateService : public BaseService {
public:
    static PlateService& instance();

    struct PlateResult {
        std::string plate_number;
        double confidence;
        std::string color;
        std::string message;
    };

    PlateResult recognize(const std::string& image_data);
    static bool validatePlate(const std::string& plate);
private:
    PlateService() = default;
};
