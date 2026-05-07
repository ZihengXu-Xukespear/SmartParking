#include "plate_service.h"

PlateService& PlateService::instance() {
    static PlateService inst;
    return inst;
}

PlateService::PlateResult PlateService::recognize(const std::string& image_data) {
    PlateResult result;
    result.plate_number = "";
    result.confidence = 0.0;
    result.color = "";
    result.message = "车牌识别接口已预留，OpenCV/EasyPR 尚未集成。"
                    "请通过手动输入车牌号或集成识别引擎后使用此功能。";
    return result;
}

bool PlateService::validatePlate(const std::string& plate) {
    if (plate.size() < 9 || plate.size() > 10) return false;

    const std::string provinces = "京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤川青藏琼宁";
    for (size_t i = 0; i < provinces.size(); i += 3) {
        if (plate.substr(0, 3) == provinces.substr(i, 3)) {
            if (plate[3] >= 'A' && plate[3] <= 'Z')
                return true;
        }
    }
    return false;
}
