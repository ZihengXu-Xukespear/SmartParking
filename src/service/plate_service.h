#pragma once
#include <string>
#include "base_service.h"

// OpenCV/车牌识别预留接口
// 后续可集成 EasyPR 或其他车牌识别引擎
class PlateService : public BaseService {
public:
    static PlateService& instance() {
        static PlateService inst;
        return inst;
    }

    struct PlateResult {
        std::string plate_number;
        double confidence;
        std::string color;
        std::string message;
    };

    // 预留接口：接收图片数据，返回识别结果
    // 目前返回模拟数据，后续替换为实际 OpenCV + EasyPR 实现
    PlateResult recognize(const std::string& image_data) {
        PlateResult result;
        result.plate_number = "";
        result.confidence = 0.0;
        result.color = "";
        result.message = "车牌识别接口已预留，OpenCV/EasyPR 尚未集成。"
                        "请通过手动输入车牌号或集成识别引擎后使用此功能。";

        // TODO: 集成 OpenCV + EasyPR 后，替换以下代码：
        // 1. cv::Mat img = cv::imdecode(cv::Mat(1, image_data.size(), CV_8UC1, (void*)image_data.data()), cv::IMREAD_COLOR);
        // 2. easypr::api::plate_recognize(img, results)
        // 3. 解析识别结果并填充 PlateResult

        return result;
    }

    // 验证车牌号格式（统一的校验逻辑，所有校验入口应复用此方法）
    static bool validatePlate(const std::string& plate) {
        // UTF-8 Chinese province abbreviation (3 bytes) + letter (1 byte) + 5-6 alphanumeric
        // Standard plate: 京A12345 = 3+1+5 = 9 bytes
        // New energy plate: 京A123456 = 3+1+6 = 10 bytes
        if (plate.size() < 9 || plate.size() > 10) return false;

        const std::string provinces = "京津沪渝冀豫云辽黑湘皖鲁新苏浙赣鄂桂甘晋蒙陕吉闽贵粤川青藏琼宁";
        for (size_t i = 0; i < provinces.size(); i += 3) {
            if (plate.substr(0, 3) == provinces.substr(i, 3)) {
                // Province letter (A-Z) at byte index 3
                if (plate[3] >= 'A' && plate[3] <= 'Z')
                    return true;
            }
        }
        return false;
    }

private:
    PlateService() = default;
};
