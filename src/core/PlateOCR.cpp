#include "core/PlateOCR.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <cmath>

// ============================================================
// 字符集定义 (78个字符)
// index 0 = '#' 是 CTC blank token
// index 1~31 = 省份简称 (31个)
// index 32~41 = 特殊标记 (10个)
// index 42~51 = 数字 0-9 (10个)
// index 52~75 = 字母 A-Z 去掉I和O (24个)
// index 76~77 = 险品 (2个)
// ============================================================
const char* PlateOCR::PLATE_CHARS =
    "#"
    "\xe4\xba\xac\xe6\xb2\xaa\xe6\xb4\xa5\xe6\xb8\x9d"  // 京沪津渝
    "\xe5\x86\x80\xe6\x99\x8b\xe8\x92\x99\xe8\xbe\xbd"  // 冀晋蒙辽
    "\xe5\x90\x89\xe9\xbb\x91\xe8\x8b\x8f\xe6\xb5\x99"  // 吉黑苏浙
    "\xe7\x9a\x96\xe9\x97\xbd\xe8\xb5\xa3\xe9\xb2\x81"  // 皖闽赣鲁
    "\xe8\xb1\xab\xe9\x84\x82\xe6\xb9\x98\xe7\xb2\xa4"  // 豫鄂湘粤
    "\xe6\xa1\x82\xe7\x90\xbc\xe5\xb7\x9d\xe8\xb4\xb5"  // 桂琼川贵
    "\xe4\xba\x91\xe8\x97\x8f\xe9\x99\x95\xe7\x94\x98"  // 云藏陕甘
    "\xe9\x9d\x92\xe5\xae\x81\xe6\x96\xb0"                // 青宁新
    "\xe5\xad\xa6\xe8\xad\xa6\xe6\xb8\xaf\xe6\xbe\xb3"  // 学警港澳
    "\xe6\x8c\x82\xe4\xbd\xbf\xe9\xa2\x86\xe6\xb0\x91"  // 挂使领民
    "\xe8\x88\xaa\xe5\x8d\xb1"                                // 航危
    "0123456789"
    "ABCDEFGHJKLMNPQRSTUVWXYZ"
    "\xe9\x99\xa9\xe5\x93\x81";                                // 险品

// ============================================================
// 构造函数
// ============================================================
PlateOCR::PlateOCR() : m_loaded(false) {
}

// ============================================================
// 加载ONNX模型
// ============================================================
bool PlateOCR::loadModel(const QString &modelPath) {
    m_loaded = false;
    m_lastError.clear();

    QStringList paths;
    paths << modelPath;

    QString appDir = QCoreApplication::applicationDirPath();
    paths << QDir(appDir).filePath("models/plate_rec.onnx");
    paths << QDir(appDir).filePath("../models/plate_rec.onnx");
    paths << "D:/Arctic_QT/PlateRecognition/models/plate_rec.onnx";

    for (const QString &path : paths) {
        if (!QFileInfo::exists(path)) {
            continue;
        }

        std::string stdPath = path.toUtf8().constData();
        qDebug() << "PlateOCR: loading model from" << path;

        try {
            m_net = cv::dnn::readNetFromONNX(stdPath);

            if (m_net.empty()) {
                m_lastError = "Model loaded but network is empty";
                continue;
            }

            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

            m_loaded = true;
            qDebug() << "PlateOCR: model loaded OK from" << path;
            return true;

        } catch (const cv::Exception &e) {
            m_lastError = QString("OpenCV error: %1").arg(e.what());
            qWarning() << "PlateOCR: failed to load from" << path << "-" << e.what();
        }
    }

    if (m_lastError.isEmpty()) {
        m_lastError = "plate_rec.onnx not found in any search path";
    }
    qCritical() << "PlateOCR:" << m_lastError;
    return false;
}

// ============================================================
// 预处理: 车牌裁剪图 -> 模型输入tensor
// 1. Resize 到 168x48
// 2. BGR -> RGB
// 3. 归一化: (pixel/255 - 0.588) / 0.193
// 4. HWC -> CHW, shape = [1, 3, 48, 168]
// ============================================================
cv::Mat PlateOCR::preprocess(const cv::Mat &plateCrop) {
    cv::Mat resized;
    cv::resize(plateCrop, resized, cv::Size(INPUT_WIDTH, INPUT_HEIGHT), 0, 0, cv::INTER_CUBIC);

    // BGR -> RGB
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    // 转为 float 并归一化: (x/255 - mean) / std
    cv::Mat floatImg;
    rgb.convertTo(floatImg, CV_32FC3, 1.0 / 255.0);
    floatImg = (floatImg - MEAN_VALUE) / STD_VALUE;

    // HWC -> CHW blob: [1, 3, 48, 168]
    cv::Mat blob = cv::dnn::blobFromImage(floatImg);
    return blob;
}

// ============================================================
// CTC 贪心解码
// 模型输出 shape: [1, 21, 78]
// 21个时间步, 每个时间步78个字符的logit
// 规则: 取argmax, 去掉blank(0)和连续重复, 映射到字符
// ============================================================
QString PlateOCR::ctcDecode(const cv::Mat &output, double &confidence) {
    confidence = 0.0;

    int seqLen = output.size[1];
    int numClasses = output.size[2];
    const float *data = output.ptr<float>(0);

    // 将字符集转为 QString (UTF-8 -> Unicode)
    QString charStr = QString::fromUtf8(PLATE_CHARS);

    std::vector<int> indices;
    std::vector<float> probs;
    int prevIdx = -1;

    for (int t = 0; t < seqLen; t++) {
        const float *timestep = data + t * numClasses;

        // 找最大 logit 索引
        int maxIdx = 0;
        float maxLogit = timestep[0];
        for (int c = 1; c < numClasses; c++) {
            if (timestep[c] > maxLogit) {
                maxLogit = timestep[c];
                maxIdx = c;
            }
        }

        // CTC规则: 跳过blank(0)和连续重复
        if (maxIdx != 0 && maxIdx != prevIdx) {
            // sigmoid 近似概率
            float prob = 1.0f / (1.0f + std::exp(-maxLogit));
            indices.push_back(maxIdx);
            probs.push_back(prob);
        }
        prevIdx = maxIdx;
    }

    if (indices.empty()) {
        return QString();
    }

    // 计算平均置信度
    double totalProb = 0.0;
    for (float p : probs) {
        totalProb += p;
    }
    confidence = totalProb / probs.size();

    // 将索引映射为字符
    QString result;
    for (int idx : indices) {
        if (idx >= 0 && idx < charStr.length()) {
            result.append(charStr[idx]);
        }
    }

    return result;
}

// ============================================================
// 识别接口: 输入BGR车牌裁剪图, 输出车牌号+置信度
// ============================================================
QString PlateOCR::recognize(const cv::Mat &plateCrop, double &confidence) {
    confidence = 0.0;

    if (!m_loaded) {
        m_lastError = "Model not loaded";
        return QString();
    }

    if (plateCrop.empty()) {
        m_lastError = "Input image is empty";
        return QString();
    }

    // 预处理
    cv::Mat blob = preprocess(plateCrop);
    m_net.setInput(blob);

    // 前向推理
    cv::Mat output = m_net.forward();

    // CTC解码
    QString result = ctcDecode(output, confidence);

    if (result.isEmpty()) {
        m_lastError = "CTC decode failed: no valid characters";
    }

    return result;
}
