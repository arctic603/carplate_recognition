#include "core/PlateDetector.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>
#include <cmath>
#include <algorithm>

// ============================================================
// 构造函数
// ============================================================
PlateDetector::PlateDetector() : m_yoloLoaded(false) {
    // 尝试自动加载YOLO模型
    loadYoloModel("models/car_plate_detect.onnx");
}

// ============================================================
// 加载YOLO检测模型
// ============================================================
bool PlateDetector::loadYoloModel(const QString &modelPath) {
    m_yoloLoaded = false;

    QStringList paths;
    paths << modelPath;

    QString appDir = QCoreApplication::applicationDirPath();
    paths << QDir(appDir).filePath("models/car_plate_detect.onnx");
    paths << QDir(appDir).filePath("../models/car_plate_detect.onnx");
    paths << "D:/Arctic_QT/PlateRecognition/models/car_plate_detect.onnx";

    for (const QString &path : paths) {
        if (!QFileInfo::exists(path)) continue;

        try {
            m_yoloNet = cv::dnn::readNetFromONNX(path.toUtf8().constData());
            if (m_yoloNet.empty()) continue;

            m_yoloNet.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
            m_yoloNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            m_yoloLoaded = true;
            qDebug() << "YOLO plate detector loaded from" << path;
            return true;
        } catch (const cv::Exception &e) {
            qWarning() << "YOLO load failed from" << path << ":" << e.what();
        }
    }

    qDebug() << "YOLO model not found, using morphological detection only";
    return false;
}

// ============================================================
// Letterbox resize (保持宽高比, 灰色填充)
// ============================================================
void PlateDetector::letterbox(const cv::Mat &img, cv::Mat &out,
                               double &scale, int &padX, int &padY,
                               int targetSize) {
    int h = img.rows, w = img.cols;
    scale = std::min((double)targetSize / h, (double)targetSize / w);
    int newH = (int)(h * scale), newW = (int)(w * scale);

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(newW, newH));

    out = cv::Mat(targetSize, targetSize, CV_8UC3, cv::Scalar(114, 114, 114));
    padX = (targetSize - newW) / 2;
    padY = (targetSize - newH) / 2;
    resized.copyTo(out(cv::Rect(padX, padY, newW, newH)));
}

// ============================================================
// YOLO 检测
// 输出格式: [cx,cy,w,h,obj, x1,y1,x2,y2,x3,y3,x4,y4, p_blue,p_yellow,p_green]
// ============================================================
std::vector<PlateRegion> PlateDetector::detectYolo(const cv::Mat &image) {
    std::vector<PlateRegion> results;
    if (!m_yoloLoaded) return results;

    int origH = image.rows, origW = image.cols;

    // 预处理: letterbox + normalize
    cv::Mat padded;
    double scale;
    int padX, padY;
    letterbox(image, padded, scale, padX, padY, YOLO_INPUT_SIZE);

    cv::Mat blob = cv::dnn::blobFromImage(padded, 1.0 / 255.0,
                                           cv::Size(YOLO_INPUT_SIZE, YOLO_INPUT_SIZE),
                                           cv::Scalar(), true); // swapRB=true

    m_yoloNet.setInput(blob);
    cv::Mat output = m_yoloNet.forward(); // [1, 25200, 16]

    const float *data = output.ptr<float>(0);
    int numDet = output.size[1]; // 25200
    int numFields = output.size[2]; // 16

    // 收集所有候选
    struct YoloDet {
        int x, y, w, h;
        float score;
    };
    std::vector<YoloDet> yoloDets;

    for (int i = 0; i < numDet; i++) {
        const float *det = data + i * numFields;
        float objConf = det[4];
        if (objConf < YOLO_CONF_THRESH) continue;

        // 类别概率 (位置13,14,15)
        float pBlue = det[13], pYellow = det[14], pGreen = det[15];
        float maxProb = std::max({pBlue, pYellow, pGreen});
        float score = objConf * maxProb;
        if (score < 0.2f) continue;

        // bbox 在640x640空间
        float cx640 = det[0], cy640 = det[1], w640 = det[2], h640 = det[3];

        // 还原到原图坐标
        float cx = (float)((cx640 - padX) / scale);
        float cy = (float)((cy640 - padY) / scale);
        float w = (float)(w640 / scale);
        float h = (float)(h640 / scale);

        int x1 = std::max(0, (int)(cx - w / 2));
        int y1 = std::max(0, (int)(cy - h / 2));
        int x2 = std::min(origW - 1, (int)(cx + w / 2));
        int y2 = std::min(origH - 1, (int)(cy + h / 2));

        int bw = x2 - x1, bh = y2 - y1;
        if (bw < 20 || bh < 10) continue;

        float aspect = (float)bw / bh;

        // 宽高比过滤: 只保留类似车牌比例的检测
        if (aspect < MIN_ASPECT || aspect > MAX_ASPECT) continue;

        yoloDets.push_back({x1, y1, bw, bh, score});
    }

    // NMS: 按score排序, 去除重叠
    std::sort(yoloDets.begin(), yoloDets.end(),
              [](const YoloDet &a, const YoloDet &b) { return a.score > b.score; });

    for (size_t i = 0; i < yoloDets.size(); i++) {
        bool overlap = false;
        for (size_t j = 0; j < i; j++) {
            // 简化的NMS: 中心距离判断
            int cx1 = yoloDets[i].x + yoloDets[i].w / 2;
            int cy1 = yoloDets[i].y + yoloDets[i].h / 2;
            int cx2 = yoloDets[j].x + yoloDets[j].w / 2;
            int cy2 = yoloDets[j].y + yoloDets[j].h / 2;
            float dist = std::sqrt((float)((cx1-cx2)*(cx1-cx2) + (cy1-cy2)*(cy1-cy2)));
            float maxDim = (float)std::max({yoloDets[i].w, yoloDets[i].h,
                                            yoloDets[j].w, yoloDets[j].h});
            if (dist < maxDim * 0.3f) {
                overlap = true;
                break;
            }
        }
        if (overlap) continue;

        // 构造 PlateRegion
        cv::Rect bbox(yoloDets[i].x, yoloDets[i].y,
                       yoloDets[i].w, yoloDets[i].h);
        PlateRegion region;
        region.boundingBox = bbox;
        region.plateImage = image(bbox).clone();
        cv::cvtColor(region.plateImage, region.plateGray, cv::COLOR_BGR2GRAY);
        region.aspectRatio = (double)bbox.width / bbox.height;
        region.confidence = yoloDets[i].score;
        region.fromYolo = true;
        results.push_back(region);

        // 最多保留3个YOLO检测结果
        if (results.size() >= 3) break;
    }

    qDebug() << "YOLO detected" << results.size() << "plate candidates";
    return results;
}

// ============================================================
// 形态学检测 (保留原有逻辑作为兜底)
// ============================================================
std::vector<PlateRegion> PlateDetector::detectMorphological(const cv::Mat &image) {
    std::vector<PlateRegion> results;

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Sobel 边缘检测
    cv::Mat sobelX;
    cv::Sobel(gray, sobelX, CV_16S, 1, 0, 3);
    cv::Mat absX;
    cv::convertScaleAbs(sobelX, absX);

    // 自适应阈值
    cv::Mat binary;
    cv::threshold(absX, binary, 0, 255,
                  cv::THRESH_BINARY | cv::THRESH_OTSU);

    // 多尺度形态学操作
    int imgArea = image.rows * image.cols;
    std::vector<int> kernelScales = {1, 2, 3};
    int baseKernelW = std::max(5, (int)(std::max(image.cols, image.rows) / 720.0 * 15));
    int baseKernelH = std::max(2, baseKernelW / 5);

    std::vector<cv::Rect> allCandidates;

    for (int s : kernelScales) {
        int kw = baseKernelW * s;
        int kh = baseKernelH * s;
        cv::Mat kernel = cv::getStructuringElement(
            cv::MORPH_RECT, cv::Size(kw, kh));

        cv::Mat morph;
        cv::morphologyEx(binary, morph, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(morph, morph, cv::MORPH_OPEN,
                          cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(morph, contours, cv::RETR_EXTERNAL,
                          cv::CHAIN_APPROX_SIMPLE);

        for (const auto &contour : contours) {
            cv::Rect rect = cv::boundingRect(contour);
            double aspect = (double)rect.width / std::max(rect.height, 1);
            int area = rect.width * rect.height;

            if (area < imgArea * 0.002 || area > imgArea * 0.5) continue;
            if (aspect < 2.0 || aspect > 6.0) continue;
            if (rect.width < 60 || rect.height < 15) continue;

            allCandidates.push_back(rect);
        }
    }

    // HSV蓝色兜底检测
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    cv::Mat blueMask;
    cv::inRange(hsv, cv::Scalar(95, 50, 40),
                cv::Scalar(130, 255, 255), blueMask);

    std::vector<std::vector<cv::Point>> blueContours;
    cv::findContours(blueMask, blueContours, cv::RETR_EXTERNAL,
                      cv::CHAIN_APPROX_SIMPLE);

    for (const auto &contour : blueContours) {
        cv::Rect rect = cv::boundingRect(contour);
        double aspect = (double)rect.width / std::max(rect.height, 1);
        int area = rect.width * rect.height;

        if (area < imgArea * 0.002 || area > imgArea * 0.5) continue;
        if (aspect < 2.0 || aspect > 6.0) continue;
        if (rect.width < 60 || rect.height < 15) continue;

        allCandidates.push_back(rect);
    }

    // 去重 (IoU > 0.5 视为重复)
    std::sort(allCandidates.begin(), allCandidates.end(),
              [](const cv::Rect &a, const cv::Rect &b) {
                  return a.area() > b.area();
              });

    std::vector<cv::Rect> uniqueCandidates;
    for (const auto &rect : allCandidates) {
        bool dup = false;
        for (const auto &u : uniqueCandidates) {
            cv::Rect inter = rect & u;
            if (inter.area() > 0) {
                double iou = (double)inter.area() /
                             (rect.area() + u.area() - inter.area());
                if (iou > 0.5) { dup = true; break; }
            }
        }
        if (!dup) uniqueCandidates.push_back(rect);
    }

    // 验证 + 构造结果
    for (const auto &rect : uniqueCandidates) {
        cv::Mat bgrRegion = image(rect);
        int score = verifyCandidate(bgrRegion);
        if (score < 1) continue;

        // 尝试蓝色精细化
        cv::Rect refined = refineBlueRegion(image, rect);
        cv::Mat finalRegion = image(refined);

        PlateRegion region;
        region.boundingBox = refined;
        region.plateImage = finalRegion.clone();
        cv::cvtColor(region.plateImage, region.plateGray, cv::COLOR_BGR2GRAY);
        region.aspectRatio = (double)refined.width / refined.height;
        region.confidence = score / 3.0;
        region.fromYolo = false;
        results.push_back(region);
    }

    // 按score排序
    std::sort(results.begin(), results.end(),
              [](const PlateRegion &a, const PlateRegion &b) {
                  return a.confidence > b.confidence;
              });

    qDebug() << "Morphological detected" << results.size() << "plate candidates";
    return results;
}

// ============================================================
// 候选验证打分 (蓝色比例 + 中心蓝色 + 边缘密度)
// ============================================================
int PlateDetector::verifyCandidate(const cv::Mat &bgrRegion) {
    if (bgrRegion.rows < 10 || bgrRegion.cols < 10) return 0;

    int score = 0;
    int area = bgrRegion.rows * bgrRegion.cols;

    // 蓝色比例
    cv::Mat hsv;
    cv::cvtColor(bgrRegion, hsv, cv::COLOR_BGR2HSV);
    cv::Mat blueMask;
    cv::inRange(hsv, cv::Scalar(95, 50, 40),
                cv::Scalar(130, 255, 255), blueMask);
    double blueRatio = (double)cv::countNonZero(blueMask) / area;
    if (blueRatio > 0.25) score++;

    // 中心蓝色
    int cx = (int)(bgrRegion.cols * 0.2);
    int cy = (int)(bgrRegion.rows * 0.2);
    int cw = (int)(bgrRegion.cols * 0.6);
    int ch = (int)(bgrRegion.rows * 0.6);
    cv::Mat center = blueMask(cv::Rect(cx, cy, cw, ch));
    double centerBlue = (double)cv::countNonZero(center) / (cw * ch);
    if (centerBlue > 0.15) score++;

    // 垂直边缘密度
    cv::Mat gray;
    cv::cvtColor(bgrRegion, gray, cv::COLOR_BGR2GRAY);
    cv::Mat sobelX;
    cv::Sobel(gray, sobelX, CV_64F, 1, 0, 3);
    cv::Mat absX;
    cv::convertScaleAbs(sobelX, absX);
    double edgeCount = cv::countNonZero(absX > 40);
    double edgeRatio = edgeCount / area;
    if (edgeRatio > 0.03) score++;

    return score;
}

// ============================================================
// 蓝色区域精细化
// ============================================================
cv::Rect PlateDetector::refineBlueRegion(const cv::Mat &bgrImage,
                                           const cv::Rect &candidateRect) {
    cv::Mat hsv;
    cv::cvtColor(bgrImage, hsv, cv::COLOR_BGR2HSV);

    cv::Mat blueMask;
    cv::inRange(hsv, cv::Scalar(100, 70, 50),
                cv::Scalar(124, 255, 255), blueMask);

    // 只看候选区域内
    cv::Mat roi = blueMask(candidateRect);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(roi, contours, cv::RETR_EXTERNAL,
                      cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) return candidateRect;

    // 找最大蓝色连通域
    double maxArea = 0;
    cv::Rect bestRect = candidateRect;
    for (const auto &c : contours) {
        cv::Rect r = cv::boundingRect(c);
        double a = r.area();
        if (a > maxArea) {
            maxArea = a;
            // 偏移回原图坐标
            bestRect = cv::Rect(r.x + candidateRect.x,
                                r.y + candidateRect.y,
                                r.width, r.height);
        }
    }

    // 验证精细化结果合理性
    double aspect = (double)bestRect.width / std::max(bestRect.height, 1);
    if (aspect >= 2.0 && aspect <= 6.0 &&
        bestRect.width >= 60 && bestRect.height >= 15) {
        return bestRect;
    }

    return candidateRect;
}

// ============================================================
// 主检测接口: YOLO优先, 形态学兜底
// ============================================================
std::vector<PlateRegion> PlateDetector::detect(const cv::Mat &image) {
    std::vector<PlateRegion> results;

    // 策略1: YOLO检测 (对全车照片效果好)
    if (m_yoloLoaded) {
        results = detectYolo(image);
    }

    // 策略2: 如果YOLO没找到有效结果, 用形态学兜底
    if (results.empty()) {
        qDebug() << "YOLO found nothing valid, falling back to morphological";
        results = detectMorphological(image);
    }

    // 最终排序: 按置信度降序
    std::sort(results.begin(), results.end(),
              [](const PlateRegion &a, const PlateRegion &b) {
                  // 优先YOLO结果 (同置信度下)
                  if (std::abs(a.confidence - b.confidence) < 0.05) {
                      return a.fromYolo > b.fromYolo;
                  }
                  return a.confidence > b.confidence;
              });

    return results;
}
