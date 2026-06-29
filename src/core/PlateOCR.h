#ifndef PLATEOCR_H
#define PLATEOCR_H

#include <QString>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <vector>

// ============================================================
// PlateOCR - 基于ONNX深度学习模型的车牌文字识别
// 使用 OnnxOCR 的 plate_rec.onnx 模型 + OpenCV cv::dnn 推理
// 输入: 裁剪后的车牌彩色图片 (BGR)
// 输出: 车牌号码字符串 + 置信度
// ============================================================
class PlateOCR {
public:
    PlateOCR();

    // 加载ONNX模型, 返回是否成功
    bool loadModel(const QString &modelPath);

    // 模型是否已加载
    bool isLoaded() const { return m_loaded; }

    // 识别车牌文字 (输入BGR彩色裁剪图)
    // plateCrop: 裁剪后的车牌区域 (BGR)
    // confidence: 输出置信度 (0~1)
    // 返回: 车牌号码字符串, 失败返回空字符串
    QString recognize(const cv::Mat &plateCrop, double &confidence);

    // 获取上次错误信息
    QString lastError() const { return m_lastError; }

private:
    // 预处理: 将车牌裁剪图转为模型输入tensor
    cv::Mat preprocess(const cv::Mat &plateCrop);

    // CTC贪心解码: 将模型输出解码为文字
    QString ctcDecode(const cv::Mat &output, double &confidence);

    cv::dnn::Net m_net;
    bool m_loaded;
    QString m_lastError;

    // 模型参数
    static constexpr int INPUT_WIDTH  = 168;
    static constexpr int INPUT_HEIGHT = 48;
    static constexpr float MEAN_VALUE = 0.588f;
    static constexpr float STD_VALUE  = 0.193f;

    // 字符集 (78个字符, index 0 是 CTC blank)
    static const char* PLATE_CHARS;
};

#endif // PLATEOCR_H
