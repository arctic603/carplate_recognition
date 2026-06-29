#ifndef PLATERECOGNIZER_H
#define PLATERECOGNIZER_H

#include <QObject>
#include <QString>
#include <QMetaType>
#include <opencv2/core.hpp>
#include "core/PlateDetector.h"
#include "core/PlateOCR.h"

// 识别结果结构体
struct RecognitionResult {
    bool success = false;
    QString plateNumber;
    double confidence = 0.0;
    cv::Mat processedImage;       // 标注了车牌框的图
    cv::Mat plateImage;           // 裁剪的车牌图
    std::vector<PlateRegion> candidates;
    QString errorMessage;
};

class PlateRecognizer : public QObject {
    Q_OBJECT
public:
    explicit PlateRecognizer(QObject *parent = nullptr);

    RecognitionResult recognize(const cv::Mat &inputImage);
    RecognitionResult recognizeFromFile(const QString &filePath);

    // 获取ONNX模型状态
    bool isModelLoaded() const { return m_ocr.isLoaded(); }

signals:
    void recognitionStarted();
    void recognitionFinished(const RecognitionResult &result);
    void progressChanged(int percent, const QString &stage);

private:
    PlateDetector m_detector;
    PlateOCR m_ocr;
    bool m_initialized;
};

// 注册元类型, 使 RecognitionResult 可以通过信号槽传递
Q_DECLARE_METATYPE(RecognitionResult)

#endif
