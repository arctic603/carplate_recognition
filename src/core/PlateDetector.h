#ifndef PLATEDETECTOR_H
#define PLATEDETECTOR_H

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <QString>

// 车牌区域检测结果
struct PlateRegion {
    cv::Rect boundingBox;     // 在原图中的矩形区域
    cv::Mat plateImage;       // 裁剪后的BGR彩色图
    cv::Mat plateGray;        // 裁剪后的灰度图
    double aspectRatio;       // 宽高比
    double confidence;        // 检测置信度 (0~1)
    bool fromYolo;            // 是否来自YOLO检测
};

class PlateDetector {
public:
    PlateDetector();

    // 加载YOLO检测模型 (可选, 不加载则只用形态学)
    bool loadYoloModel(const QString &modelPath);
    bool isYoloLoaded() const { return m_yoloLoaded; }

    // 检测车牌区域 (混合策略: YOLO优先, 形态学兜底)
    std::vector<PlateRegion> detect(const cv::Mat &image);

private:
    // YOLO 检测
    std::vector<PlateRegion> detectYolo(const cv::Mat &image);

    // 形态学检测 (原有方案, 作为兜底)
    std::vector<PlateRegion> detectMorphological(const cv::Mat &image);

    // 候选区域验证打分
    int verifyCandidate(const cv::Mat &bgrRegion);

    // 蓝色区域精细化
    cv::Rect refineBlueRegion(const cv::Mat &bgrImage, const cv::Rect &candidateRect);

    // YOLO letterbox预处理
    void letterbox(const cv::Mat &img, cv::Mat &out,
                   double &scale, int &padX, int &padY, int targetSize = 640);

    cv::dnn::Net m_yoloNet;
    bool m_yoloLoaded;

    // YOLO 参数
    static constexpr int YOLO_INPUT_SIZE = 640;
    static constexpr float YOLO_CONF_THRESH = 0.25f;
    static constexpr float MIN_ASPECT = 2.0f;
    static constexpr float MAX_ASPECT = 6.0f;
};

#endif // PLATEDETECTOR_H
