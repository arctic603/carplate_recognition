#include "core/PlateRecognizer.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QFileInfo>
#include <QDebug>
#include <cmath>
#include <algorithm>

// ------------------------------------------------------------------
// 构造函数: 初始化ONNX模型
// ------------------------------------------------------------------
PlateRecognizer::PlateRecognizer(QObject *parent)
    : QObject(parent), m_initialized(false) {

    qRegisterMetaType<RecognitionResult>("RecognitionResult");

    bool loaded = m_ocr.loadModel("models/plate_rec.onnx");
    if (!loaded) {
        qWarning() << "PlateOCR model load failed:" << m_ocr.lastError();
    }

    m_initialized = loaded;
    qDebug() << "PlateRecognizer initialized, model loaded:" << loaded;
}

// ------------------------------------------------------------------
// 验证车牌号是否合法
// 标准蓝牌: 7字符 (省份+字母+5位)
// 新能源牌: 8字符
// 最低要求: >= 7字符 且 首字符为中文省份简称
// ------------------------------------------------------------------
static bool isValidPlate(const QString &plate) {
    if (plate.length() < 7 || plate.length() > 8) {
        return false;
    }

    // 首字符应该是中文省份简称
    // 常见省份: 京沪津渝冀晋蒙辽吉黑苏浙皖闽赣鲁豫鄂湘粤桂琼川贵云藏陕甘青宁新
    static const QString provinces =
        QString::fromUtf8(
            "\xe4\xba\xac" // 京
            "\xe6\xb2\xaa" // 沪
            "\xe6\xb4\xa5" // 津
            "\xe6\xb8\x9d" // 渝
            "\xe5\x86\x80" // 冀
            "\xe6\x99\x8b" // 晋
            "\xe8\x92\x99" // 蒙
            "\xe8\xbe\xbd" // 辽
            "\xe5\x90\x89" // 吉
            "\xe9\xbb\x91" // 黑
            "\xe8\x8b\x8f" // 苏
            "\xe6\xb5\x99" // 浙
            "\xe7\x9a\x96" // 皖
            "\xe9\x97\xbd" // 闽
            "\xe8\xb5\xa3" // 赣
            "\xe9\xb2\x81" // 鲁
            "\xe8\xb1\xab" // 豫
            "\xe9\x84\x82" // 鄂
            "\xe6\xb9\x98" // 湘
            "\xe7\xb2\xa4" // 粤
            "\xe6\xa1\x82" // 桂
            "\xe7\x90\xbc" // 琼
            "\xe5\xb7\x9d" // 川
            "\xe8\xb4\xb5" // 贵
            "\xe4\xba\x91" // 云
            "\xe8\x97\x8f" // 藏
            "\xe9\x99\x95" // 陕
            "\xe7\x94\x98" // 甘
            "\xe9\x9d\x92" // 青
            "\xe5\xae\x81" // 宁
            "\xe6\x96\xb0" // 新
        );

    QChar first = plate[0];
    if (!provinces.contains(first)) {
        return false;
    }

    // 第2字符应为字母 A-Z
    QChar second = plate[1];
    if (second < QChar('A') || second > QChar('Z')) {
        return false;
    }

    return true;
}

// ------------------------------------------------------------------
// 识别主流程 (改进版: 多候选 + OCR验证 + 回退)
// ------------------------------------------------------------------
RecognitionResult PlateRecognizer::recognize(const cv::Mat &inputImage) {
    RecognitionResult result;

    if (!m_initialized) {
        result.errorMessage = "Recognizer not initialized (ONNX model not loaded)";
        return result;
    }

    if (inputImage.empty()) {
        result.errorMessage = "Input image is empty";
        return result;
    }

    emit recognitionStarted();
    emit progressChanged(10, "Detecting plate regions...");

    // Step 1: 检测所有候选区域
    std::vector<PlateRegion> candidates = m_detector.detect(inputImage);
    result.candidates = candidates;

    if (candidates.empty()) {
        result.errorMessage = "No plate regions detected";
        emit progressChanged(100, "Recognition failed");
        emit recognitionFinished(result);
        return result;
    }

    emit progressChanged(30, QString("Found %1 candidates, ONNX recognition...").arg(candidates.size()));

    // Step 2: 按宽高比排序候选 (最接近标准车牌 3.14 的排前面)
    const double idealRatio = 3.14;
    std::vector<int> sortedIdx(candidates.size());
    std::iota(sortedIdx.begin(), sortedIdx.end(), 0);
    std::sort(sortedIdx.begin(), sortedIdx.end(), [&](int a, int b) {
        return std::abs(candidates[a].aspectRatio - idealRatio) <
               std::abs(candidates[b].aspectRatio - idealRatio);
    });

    // Step 3: 逐候选做 OCR, 找第一个通过验证的
    QString bestPlate;
    double bestConfidence = 0.0;
    int bestCandidateIdx = sortedIdx[0]; // 默认用比例最好的
    bool foundValid = false;

    for (int rank = 0; rank < (int)sortedIdx.size(); rank++) {
        int idx = sortedIdx[rank];
        const PlateRegion &candidate = candidates[idx];

        // 对候选做 OCR (需要BGR彩色图)
        double conf = 0.0;
        QString plate = m_ocr.recognize(candidate.plateImage, conf);

        qDebug() << "Candidate" << idx
                 << "ratio:" << candidate.aspectRatio
                 << "size:" << candidate.boundingBox.width << "x" << candidate.boundingBox.height
                 << "OCR:" << plate
                 << "conf:" << conf;

        // 更新最佳结果 (即使不合法也保存, 用于兜底)
        if (conf > bestConfidence && !plate.isEmpty()) {
            bestConfidence = conf;
            bestPlate = plate;
            bestCandidateIdx = idx;
        }

        // 验证: 置信度 >= 0.85 且 字符数合法 且 格式合法
        if (!plate.isEmpty() && conf >= 0.85 && plate.length() >= 7 && isValidPlate(plate)) {
            bestPlate = plate;
            bestConfidence = conf;
            bestCandidateIdx = idx;
            foundValid = true;
            qDebug() << "Valid plate found at candidate" << idx << ":" << plate;
            break; // 找到合法结果, 停止搜索
        }
    }

    // Step 4: 如果所有候选都不通过, 放宽条件再试一次 (降低置信度阈值)
    if (!foundValid && !bestPlate.isEmpty()) {
        // 检查兜底结果是否至少有7个字符
        if (bestPlate.length() >= 7) {
            qDebug() << "Using best-effort result:" << bestPlate
                     << "conf:" << bestConfidence;
        } else {
            // 字符数不够, 可能是检测裁剪不精确
            // 尝试对原图做整图OCR (最后手段)
            qDebug() << "All candidates failed validation, trying full image...";
            double fullConf = 0.0;
            QString fullPlate = m_ocr.recognize(inputImage, fullConf);
            if (!fullPlate.isEmpty() && fullPlate.length() >= 7 && isValidPlate(fullPlate)) {
                bestPlate = fullPlate;
                bestConfidence = fullConf;
                bestCandidateIdx = sortedIdx[0];
                qDebug() << "Full image OCR found:" << fullPlate;
            }
        }
    }

    if (bestPlate.isEmpty() || bestPlate.length() < 7) {
        result.errorMessage = "Recognition failed: no valid plate found (best: '"
            + bestPlate + "', conf=" + QString::number(bestConfidence, 'f', 3) + ")";
        emit progressChanged(100, "Recognition failed");
        emit recognitionFinished(result);
        return result;
    }

    // Step 5: 构造结果
    const PlateRegion &bestRegion = candidates[bestCandidateIdx];
    result.plateNumber = bestPlate;
    result.success     = true;
    result.confidence  = bestConfidence;
    result.plateImage  = bestRegion.plateImage.clone();

    // 在原图上画绿色边框
    result.processedImage = inputImage.clone();
    cv::rectangle(result.processedImage,
                  bestRegion.boundingBox,
                  cv::Scalar(0, 255, 0), 2);

    // 在框上方标注车牌号
    cv::putText(result.processedImage,
                bestPlate.toStdString(),
                cv::Point(bestRegion.boundingBox.x,
                          bestRegion.boundingBox.y - 5),
                cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(0, 255, 0), 2);

    emit progressChanged(100, "Recognition complete");
    emit recognitionFinished(result);

    qDebug() << "Final result:" << bestPlate << "conf:" << bestConfidence;
    return result;
}

// ------------------------------------------------------------------
// 从文件路径识别
// ------------------------------------------------------------------
RecognitionResult PlateRecognizer::recognizeFromFile(const QString &filePath) {
    RecognitionResult result;

    if (!QFileInfo::exists(filePath)) {
        result.errorMessage = "File not found: " + filePath;
        return result;
    }

    cv::Mat image = cv::imread(filePath.toUtf8().constData());

    if (image.empty()) {
        result.errorMessage = "Cannot read image: " + filePath;
        return result;
    }

    return recognize(image);
}
