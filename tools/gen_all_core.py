#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
车牌识别项目核心模块源文件生成脚本
生成以下6个文件：
  src/core/PlateDetector.h
  src/core/PlateDetector.cpp
  src/core/CharacterClassifier.h
  src/core/CharacterClassifier.cpp
  src/core/PlateRecognizer.h
  src/core/PlateRecognizer.cpp
"""
import os
import sys

BASE = r"D:\Arctic_QT\PlateRecognition"
CORE_DIR = os.path.join(BASE, "src", "core")


def write_file(path, content):
    """写入文件，自动创建父目录"""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"[OK] {path}")


# ============================================================
# 1. PlateDetector.h
# ============================================================
plate_detector_h = '''\
#ifndef PLATEDETECTOR_H
#define PLATEDETECTOR_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

// 车牌区域结构体
struct PlateRegion {
    cv::Rect boundingBox;       // 在原图中的位置
    cv::Mat plateImage;         // 裁剪后的车牌区域（彩色）
    cv::Mat plateGray;          // 裁剪后的车牌区域（灰度）
    double aspectRatio;         // 宽高比
};

class PlateDetector {
public:
    PlateDetector();

    // 主入口：返回所有候选车牌区域
    std::vector<PlateRegion> detect(const cv::Mat &inputImage);

    // 可调参数（方便实验）
    int gaussianKernelSize = 5;
    int morphKernelWidth = 17;
    int morphKernelHeight = 3;
    double minAspectRatio = 2.0;
    double maxAspectRatio = 5.0;
    int minContourArea = 2000;
    int maxContourArea = 100000;

private:
    cv::Mat toGrayscale(const cv::Mat &input);
    cv::Mat applyGaussianBlur(const cv::Mat &gray);
    cv::Mat applySobelEdge(const cv::Mat &blurred);
    cv::Mat applyMorphology(const cv::Mat &edges);
    std::vector<cv::Rect> findPlateContours(const cv::Mat &morphed, const cv::Mat &original);
};

#endif
'''

# ============================================================
# 2. PlateDetector.cpp
# ============================================================
plate_detector_cpp = '''\
#include "core/PlateDetector.h"
#include <opencv2/highgui.hpp>
#include <algorithm>

PlateDetector::PlateDetector() {}

// ------------------------------------------------------------------
// 灰度化
// 原理：将三通道彩色图转换为单通道灰度图。
// 边缘检测主要基于亮度变化，灰度化可以将三维问题降为一维，
// 大幅降低计算复杂度，同时保留主要的边缘信息。
// ------------------------------------------------------------------
cv::Mat PlateDetector::toGrayscale(const cv::Mat &input) {
    cv::Mat gray;
    if (input.channels() == 3) {
        // cv::cvtColor BGR2GRAY：按人眼感知权重加权三通道
        // 公式：Gray = 0.299*R + 0.587*G + 0.114*B
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    return gray;
}

// ------------------------------------------------------------------
// 高斯模糊
// 原理：使用高斯核对图像进行卷积，平滑高频噪声。
// 图像传感器噪声或 JPEG 压缩伪影会产生虚假边缘，
// 高斯模糊可以有效抑制这些干扰，避免后续边缘检测产生大量噪声。
// kernel size 越大，模糊效果越强，但也会丢失真实边缘细节。
// 需要在"降噪"和"保留边缘"之间取得平衡，默认值 5 适合多数场景。
// ------------------------------------------------------------------
cv::Mat PlateDetector::applyGaussianBlur(const cv::Mat &gray) {
    cv::Mat blurred;
    // gaussianKernelSize 必须为奇数，确保高斯核有明确的中心像素
    int ksize = (gaussianKernelSize % 2 == 0) ? gaussianKernelSize + 1 : gaussianKernelSize;
    if (ksize < 1) ksize = 1;
    // sigma=0 时 OpenCV 自动根据 kernel size 计算最优 sigma
    cv::GaussianBlur(gray, blurred, cv::Size(ksize, ksize), 0);
    return blurred;
}

// ------------------------------------------------------------------
// Sobel 边缘检测
// 原理：Sobel 算子通过计算图像梯度的水平和垂直分量来检测边缘。
// - x 方向 Sobel（dx=1, dy=0）：检测垂直边缘（亮度在水平方向剧烈变化的位置）
// - y 方向 Sobel（dx=0, dy=1）：检测水平边缘
//
// 中国车牌的字符（汉字、字母、数字）笔画以垂直线条为主，
// 产生大量垂直边缘，因此 x 方向权重设为 0.7，y 方向设为 0.3。
// 这样可以在突出主要特征的同时融入少量水平边缘辅助信息。
// ------------------------------------------------------------------
cv::Mat PlateDetector::applySobelEdge(const cv::Mat &blurred) {
    // x 方向 Sobel：CV_16S 类型防止溢出（梯度可能超出 0~255 范围）
    // ksize=3 表示使用 3x3 的 Sobel 算子
    cv::Mat gradX, gradY;
    cv::Sobel(blurred, gradX, CV_16S, 1, 0, 3);
    // y 方向 Sobel：检测水平边缘（辅助信息）
    cv::Sobel(blurred, gradY, CV_16S, 0, 1, 3);

    // convertScaleAbs：将带符号的梯度值取绝对值并转为 8 位无符号
    // Sobel 算子可能产生负值（表示梯度的方向），取绝对值得到梯度强度
    cv::Mat absGradX, absGradY;
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);

    // 加权融合：x 方向权重更高（0.7），因为车牌字符以垂直边缘为主
    // 公式：edges = 0.7 * absGradX + 0.3 * absGradY + 0
    cv::Mat edges;
    cv::addWeighted(absGradX, 0.7, absGradY, 0.3, 0, edges);
    return edges;
}

// ------------------------------------------------------------------
// 形态学处理（闭运算）
// 原理：使用矩形结构元素进行闭运算（先膨胀后腐蚀）。
// - 矩形核设计为"宽而矮"（如 17x3），匹配车牌的长条形形状
// - 宽度较大（17像素）：连接水平相邻的字符边缘
// - 高度较小（3像素）：避免垂直方向过度膨胀导致上下行粘连
// - 闭运算 MORPH_CLOSE：先膨胀填充字符间的细小间隙，再腐蚀恢复边界
// 最终效果：将分散的字符边缘连接成一个完整的连通矩形区域，
// 便于后续 findContours 检测出完整的车牌候选框。
// ------------------------------------------------------------------
cv::Mat PlateDetector::applyMorphology(const cv::Mat &edges) {
    // 创建矩形结构元素：宽而矮，匹配车牌形状特征
    cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_RECT,
        cv::Size(morphKernelWidth, morphKernelHeight)
    );
    // 闭运算 MORPH_CLOSE：先膨胀后腐蚀
    // 膨胀：扩展白色区域，填充字符间空隙
    // 腐蚀：收缩白色区域，恢复边界大小
    cv::Mat morphed;
    cv::morphologyEx(edges, morphed, cv::MORPH_CLOSE, kernel);
    return morphed;
}

// ------------------------------------------------------------------
// 轮廓检测与过滤
// 原理：在形态学处理后的图像中查找外部轮廓，
// 通过面积和宽高比两个条件过滤出候选车牌区域。
// - 面积过滤：排除噪点（太小）和整张图（太大）
// - 宽高比过滤：中国标准车牌尺寸为 440mm x 140mm，
//   宽高比约 3.14。允许 2.0~5.0 范围以适应拍摄角度和距离变化。
// ------------------------------------------------------------------
std::vector<cv::Rect> PlateDetector::findPlateContours(const cv::Mat &morphed,
                                                        const cv::Mat &original) {
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    // RETR_EXTERNAL：只检测最外层轮廓，忽略嵌套的内部轮廓
    // CHAIN_APPROX_SIMPLE：压缩水平、垂直和对角线段，只保留端点，节省内存
    cv::findContours(morphed, contours, hierarchy,
                     cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Rect> candidates;
    for (const auto &contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        double area = rect.area();
        double aspect = (rect.height > 0) ? (double)rect.width / rect.height : 0.0;

        // 面积过滤：排除噪声（太小）和无关大区域（太大）
        if (area < minContourArea || area > maxContourArea) continue;

        // 宽高比过滤：中国标准车牌宽高比约 3.14（440mm / 140mm）
        // 设置合理范围（2.0~5.0）以兼容拍摄角度偏差
        if (aspect < minAspectRatio || aspect > maxAspectRatio) continue;

        candidates.push_back(rect);
    }

    return candidates;
}

// ------------------------------------------------------------------
// 主入口：串联整个图像处理流水线
// 流程：灰度化 -> 高斯模糊 -> Sobel 边缘检测 -> 形态学闭运算 -> 轮廓检测
// 对每个候选矩形裁剪彩色图和灰度图，封装为 PlateRegion 返回
// ------------------------------------------------------------------
std::vector<PlateRegion> PlateDetector::detect(const cv::Mat &inputImage) {
    std::vector<PlateRegion> results;

    if (inputImage.empty()) return results;

    // Step 1: 灰度化 - 降低计算维度，边缘检测基于亮度变化
    cv::Mat gray = toGrayscale(inputImage);

    // Step 2: 高斯模糊 - 抑制噪声，避免虚假边缘
    cv::Mat blurred = applyGaussianBlur(gray);

    // Step 3: Sobel 边缘检测 - 提取字符垂直边缘特征
    cv::Mat edges = applySobelEdge(blurred);

    // Step 4: 形态学闭运算 - 连接字符边缘，形成连通区域
    cv::Mat morphed = applyMorphology(edges);

    // Step 5: 查找轮廓并通过面积/宽高比过滤
    std::vector<cv::Rect> rects = findPlateContours(morphed, inputImage);

    // Step 6: 对每个候选区域裁剪彩色图和灰度图
    for (const auto &rect : rects) {
        PlateRegion region;
        region.boundingBox = rect;
        region.plateImage = inputImage(rect).clone();  // 彩色图（用于显示和保存）
        region.plateGray  = gray(rect).clone();         // 灰度图（用于字符分割）
        region.aspectRatio = (rect.height > 0) ? (double)rect.width / rect.height : 0.0;
        results.push_back(region);
    }

    return results;
}
'''

# ============================================================
# 3. CharacterClassifier.h
# ============================================================
character_classifier_h = '''\
#ifndef CHARACTERCLASSIFIER_H
#define CHARACTERCLASSIFIER_H

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <map>
#include <vector>

// 单字符识别结果
struct CharacterResult {
    QString character;      // 识别出的字符
    double confidence;      // 匹配得分 (0~1)
    cv::Rect boundingBox;   // 在车牌图中的位置
};

class CharacterClassifier {
public:
    CharacterClassifier();

    // 从目录加载模板
    bool loadTemplates(const QString &templateDir);

    // 分割并识别车牌中所有字符（返回每个字符的识别结果）
    std::vector<CharacterResult> classify(const cv::Mat &plateGray);

    // 返回完整车牌号字符串
    QString recognizePlate(const cv::Mat &plateGray);

private:
    std::map<QString, cv::Mat> m_provinceTemplates;   // 省份汉字模板
    std::map<QString, cv::Mat> m_letterTemplates;     // 字母模板 A-Z
    std::map<QString, cv::Mat> m_alphanumTemplates;   // 字母+数字模板

    std::vector<cv::Mat> segmentCharacters(const cv::Mat &plateGray);
    CharacterResult matchCharacter(const cv::Mat &charImg,
                                    const std::map<QString, cv::Mat> &templates);
    cv::Mat normalizeChar(const cv::Mat &charImg, int targetWidth = 20, int targetHeight = 40);
    void loadTemplatesFromDir(const QString &dir, std::map<QString, cv::Mat> &templates);
    void generateBuiltinTemplates();
};

#endif
'''

# ============================================================
# 4. CharacterClassifier.cpp
# ============================================================
character_classifier_cpp = '''\
#include "core/CharacterClassifier.h"
#include <algorithm>
#include <QDebug>

CharacterClassifier::CharacterClassifier() {}

// ------------------------------------------------------------------
// 从目录加载字符模板
// 目录结构要求：
//   templateDir/
//     provinces/   - 省份汉字模板（如 京.png, 沪.png ...）
//     letters/     - 字母模板 A-Z（如 A.png, B.png ...）
//     alphanum/    - 字母+数字模板（如 A.png, 0.png ...）
// 如果目录不存在或模板不完整，自动回退到内置模板
// ------------------------------------------------------------------
bool CharacterClassifier::loadTemplates(const QString &templateDir) {
    QDir baseDir(templateDir);
    if (!baseDir.exists()) {
        qWarning() << "模板目录不存在:" << templateDir << "，使用内置模板";
        generateBuiltinTemplates();
        return false;
    }

    // 加载省份汉字模板
    loadTemplatesFromDir(baseDir.filePath("provinces"), m_provinceTemplates);
    // 加载字母模板
    loadTemplatesFromDir(baseDir.filePath("letters"), m_letterTemplates);
    // 加载字母+数字模板
    loadTemplatesFromDir(baseDir.filePath("alphanum"), m_alphanumTemplates);

    // 如果关键模板集为空，回退到内置模板
    if (m_letterTemplates.empty() || m_alphanumTemplates.empty()) {
        qWarning() << "部分模板集为空，使用内置模板补充";
        generateBuiltinTemplates();
    }

    return true;
}

// ------------------------------------------------------------------
// 从指定目录加载模板图片
// 支持格式：PNG, JPG, BMP
// 以文件名（去掉扩展名）作为模板键名
// 图片加载后转为灰度+二值化（字符为白色前景）
// ------------------------------------------------------------------
void CharacterClassifier::loadTemplatesFromDir(const QString &dir,
                                                std::map<QString, cv::Mat> &templates) {
    QDir templateDir(dir);
    if (!templateDir.exists()) return;

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.bmp";
    QStringList files = templateDir.entryList(filters, QDir::Files, QDir::Name);

    for (const QString &file : files) {
        QString fullPath = templateDir.filePath(file);
        // Windows 下使用宽字符路径，支持中文文件名
        cv::Mat img = cv::imread(fullPath.toStdWString(), cv::IMREAD_GRAYSCALE);
        if (img.empty()) {
            qWarning() << "无法加载模板图片:" << fullPath;
            continue;
        }

        // 二值化模板：假设模板为白底黑字，反转为黑底白字（字符为白色前景）
        cv::Mat binary;
        cv::threshold(img, binary, 128, 255, cv::THRESH_BINARY_INV);

        // 以文件名（去掉扩展名）作为键名
        QString key = QFileInfo(file).baseName();
        templates[key] = binary;
    }

    qDebug() << "已加载" << (int)templates.size() << "个模板，来源:" << dir;
}

// ------------------------------------------------------------------
// 生成内置模板（无需外部图片文件）
// 使用 cv::putText 在 20x40 白色画布上绘制字符作为模板
// - 字母 A-Z 和数字 0-9 使用 FONT_HERSHEY_SIMPLEX 字体
// - 省份汉字用填充矩形占位（OpenCV 的 putText 不支持中文渲染）
// 内置模板精度有限，实际项目应替换为真实模板图片
// ------------------------------------------------------------------
void CharacterClassifier::generateBuiltinTemplates() {
    qDebug() << "生成内置字符模板...";

    // 生成字母模板 A-Z（同时也放入 alphanum 模板集，因为字母也是序号的一部分）
    for (int i = 0; i < 26; i++) {
        cv::Mat canvas(40, 20, CV_8UC1, cv::Scalar(255));  // 白色背景，20x40 像素
        char ch = (char)('A' + i);
        std::string text(1, ch);
        // FONT_HERSHEY_SIMPLEX: OpenCV 内置西文字体，适合字母和数字
        cv::putText(canvas, text, cv::Point(2, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0), 2);
        cv::Mat binary;
        cv::threshold(canvas, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QString(QChar(ch));
        m_letterTemplates[key]   = binary;
        m_alphanumTemplates[key] = binary;  // 字母也属于字母数字集
    }

    // 生成数字模板 0-9（只放入 alphanum 模板集）
    for (int i = 0; i <= 9; i++) {
        cv::Mat canvas(40, 20, CV_8UC1, cv::Scalar(255));  // 白色背景
        char ch = (char)('0' + i);
        std::string text(1, ch);
        cv::putText(canvas, text, cv::Point(2, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0), 2);
        cv::Mat binary;
        cv::threshold(canvas, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QString(QChar(ch));
        m_alphanumTemplates[key] = binary;
    }

    // 生成省份占位模板 P0-P9
    // OpenCV 的 putText 不支持中文字符渲染，使用填充矩形作为占位符
    // 实际项目中应从模板文件加载真实的省份汉字图片
    for (int i = 0; i <= 9; i++) {
        cv::Mat canvas(40, 20, CV_8UC1, cv::Scalar(255));  // 白色背景
        // 画一个填充矩形作为汉字占位符（模拟汉字的大致轮廓）
        cv::rectangle(canvas, cv::Point(2, 2), cv::Point(17, 37),
                      cv::Scalar(0), -1);  // -1 表示填充
        cv::Mat binary;
        cv::threshold(canvas, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QString("P%1").arg(i);
        m_provinceTemplates[key] = binary;
    }

    qDebug() << "内置模板生成完成:"
             << "省份" << (int)m_provinceTemplates.size()
             << "字母" << (int)m_letterTemplates.size()
             << "字母数字" << (int)m_alphanumTemplates.size();
}

// ------------------------------------------------------------------
// 字符分割
// 原理：对灰度车牌图进行自适应二值化，查找连通区域，
// 按面积和宽高比过滤出单字符区域，按 x 坐标从左到右排序。
// 中国标准车牌共 7 个字符：省份汉字 + 发牌机关代号 + 5位序号
// ------------------------------------------------------------------
std::vector<cv::Mat> CharacterClassifier::segmentCharacters(const cv::Mat &plateGray) {
    std::vector<cv::Mat> chars;

    if (plateGray.empty()) return chars;

    // 自适应二值化：适应光照不均匀的车牌图像
    // ADAPTIVE_THRESH_GAUSSIAN_C：使用高斯加权平均值作为局部阈值
    //   比 MEAN_C 更平滑，对噪声更鲁棒
    // THRESH_BINARY_INV：反转二值化（假设白底黑字），使字符为白色前景
    // blockSize=15：局部邻域大小（必须为奇数），应大于字符笔画宽度
    // C=5：从加权平均值中减去的常数，控制二值化灵敏度
    cv::Mat binary;
    cv::adaptiveThreshold(plateGray, binary, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV,
                          15, 5);

    // 查找字符轮廓（只检测最外层轮廓）
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 用 pair 存储 (x坐标, 字符图像)，以便按位置从左到右排序
    std::vector<std::pair<int, cv::Mat>> charCandidates;

    for (const auto &contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        double area   = rect.area();
        double aspect = (rect.height > 0) ? (double)rect.width / rect.height : 0.0;

        // 面积过滤：排除噪点（太小）和整个车牌（太大）
        if (area < 30 || area > plateGray.rows * plateGray.cols * 0.5) continue;

        // 单字符宽高比过滤：标准字符宽高比约 0.3~0.6
        // 放宽范围至 0.1~0.9 以兼容不同字体和磨损情况
        if (aspect < 0.1 || aspect > 0.9) continue;

        // 高度过滤：字符高度应占车牌高度的合理比例（至少 30%）
        // 排除铆钉、污渍等矮小的干扰区域
        if (rect.height < plateGray.rows * 0.3) continue;

        // 裁剪字符区域（从二值图中裁剪）
        cv::Mat charImg = binary(rect).clone();
        charCandidates.emplace_back(rect.x, charImg);
    }

    // 按 x 坐标从左到右排序（车牌字符从左到右排列）
    std::sort(charCandidates.begin(), charCandidates.end(),
              [](const std::pair<int, cv::Mat> &a, const std::pair<int, cv::Mat> &b) {
                  return a.first < b.first;
              });

    // 提取排序后的字符图像
    for (const auto &p : charCandidates) {
        chars.push_back(p.second);
    }

    // 期望找到 7 个字符区域（省份 + 字母 + 5位序号）
    if ((int)chars.size() != 7) {
        qWarning() << "字符分割数量异常：期望 7 个，实际" << (int)chars.size() << "个";
    }

    return chars;
}

// ------------------------------------------------------------------
// 字符归一化
// 将字符图像缩放到与模板相同的尺寸（默认 20x40 像素）
// 确保 matchTemplate 的输入尺寸与模板一致
// ------------------------------------------------------------------
cv::Mat CharacterClassifier::normalizeChar(const cv::Mat &charImg,
                                            int targetWidth, int targetHeight) {
    cv::Mat normalized;
    cv::resize(charImg, normalized, cv::Size(targetWidth, targetHeight));
    return normalized;
}

// ------------------------------------------------------------------
// 模板匹配识别单个字符
// 原理：将待识别字符归一化到模板尺寸后，与模板集中的每个模板
// 进行归一化相关系数匹配（TM_CCOEFF_NORMED），返回得分最高的字符。
// TM_CCOEFF_NORMED 返回值范围 [-1, 1]：
//   1  = 完美匹配
//   0  = 无相关性
//  -1  = 完全反相关
// 最终将得分映射到 [0, 1] 便于理解和使用。
// ------------------------------------------------------------------
CharacterResult CharacterClassifier::matchCharacter(
        const cv::Mat &charImg,
        const std::map<QString, cv::Mat> &templates) {
    CharacterResult best;
    best.confidence = -1.0;
    best.character  = "?";

    // 归一化到模板尺寸（20x40 像素）
    cv::Mat normalized = normalizeChar(charImg);

    for (const auto &entry : templates) {
        const cv::Mat &tmpl = entry.second;
        if (tmpl.empty()) continue;
        // matchTemplate 要求源图像不小于模板图像，尺寸必须一致
        if (normalized.size() != tmpl.size()) continue;

        // TM_CCOEFF_NORMED：归一化相关系数法
        // 对光照变化有较好的鲁棒性，返回值范围 [-1, 1]
        cv::Mat result;
        cv::matchTemplate(normalized, tmpl, result, cv::TM_CCOEFF_NORMED);
        double score = result.at<float>(0, 0);

        if (score > best.confidence) {
            best.confidence = score;
            best.character  = entry.first;
        }
    }

    // 将匹配得分从 [-1, 1] 映射到 [0, 1]
    if (best.confidence >= -1.0) {
        best.confidence = (best.confidence + 1.0) / 2.0;
    } else {
        best.confidence = 0.0;
        best.character  = "?";
    }

    return best;
}

// ------------------------------------------------------------------
// 分割并识别车牌中所有字符（返回每个字符的 CharacterResult）
// ------------------------------------------------------------------
std::vector<CharacterResult> CharacterClassifier::classify(const cv::Mat &plateGray) {
    std::vector<CharacterResult> results;
    std::vector<cv::Mat> chars = segmentCharacters(plateGray);

    for (int i = 0; i < (int)chars.size(); i++) {
        CharacterResult result;

        if (i == 0) {
            // 第1个字符：省份汉字
            result = matchCharacter(chars[i], m_provinceTemplates);
        } else if (i == 1) {
            // 第2个字符：发牌机关代号（字母）
            result = matchCharacter(chars[i], m_letterTemplates);
        } else {
            // 第3~7个字符：序号（字母或数字）
            result = matchCharacter(chars[i], m_alphanumTemplates);
        }

        results.push_back(result);
    }

    return results;
}

// ------------------------------------------------------------------
// 识别完整车牌号
// 流程：分割字符 -> 按位置选择模板集匹配 -> 拼接结果字符串
// 第1个字符用省份模板（汉字）
// 第2个字符用字母模板（A-Z，发牌机关代号）
// 第3~7个字符用字母数字模板（A-Z, 0-9，序号）
// ------------------------------------------------------------------
QString CharacterClassifier::recognizePlate(const cv::Mat &plateGray) {
    std::vector<cv::Mat> chars = segmentCharacters(plateGray);

    if (chars.empty()) {
        qWarning() << "未能分割出任何字符区域";
        return QString();
    }

    QString plateNumber;

    for (int i = 0; i < (int)chars.size(); i++) {
        CharacterResult result;

        if (i == 0) {
            // 第1个字符：省份汉字（使用省份模板）
            result = matchCharacter(chars[i], m_provinceTemplates);
            // 如果省份模板匹配度太低，尝试字母模板（兼容特殊车牌格式）
            if (result.confidence < 0.3) {
                CharacterResult alt = matchCharacter(chars[i], m_letterTemplates);
                if (alt.confidence > result.confidence) {
                    result = alt;
                }
            }
        } else if (i == 1) {
            // 第2个字符：发牌机关代号（使用字母模板 A-Z）
            result = matchCharacter(chars[i], m_letterTemplates);
        } else {
            // 第3~7个字符：序号（使用字母+数字模板 A-Z, 0-9）
            result = matchCharacter(chars[i], m_alphanumTemplates);
        }

        // 如果无法识别，用 ? 占位
        if (result.character.isEmpty() || result.character == "?") {
            plateNumber += "?";
        } else {
            plateNumber += result.character;
        }
    }

    return plateNumber;
}
'''

# ============================================================
# 5. PlateRecognizer.h
# ============================================================
plate_recognizer_h = '''\
#ifndef PLATERECOGNIZER_H
#define PLATERECOGNIZER_H

#include <QObject>
#include <QString>
#include <QMetaType>
#include <opencv2/core.hpp>
#include "core/PlateDetector.h"
#include "core/CharacterClassifier.h"

// 识别结果结构体
struct RecognitionResult {
    bool success = false;
    QString plateNumber;
    double confidence = 0.0;
    cv::Mat processedImage;       // 标注了车牌框的图
    cv::Mat plateImage;           // 裁剪的车牌图
    std::vector<PlateRegion> candidates;
    std::vector<CharacterResult> characters;
    QString errorMessage;
};

class PlateRecognizer : public QObject {
    Q_OBJECT
public:
    explicit PlateRecognizer(QObject *parent = nullptr);

    RecognitionResult recognize(const cv::Mat &inputImage);
    RecognitionResult recognizeFromFile(const QString &filePath);

signals:
    void recognitionStarted();
    void recognitionFinished(const RecognitionResult &result);
    void progressChanged(int percent, const QString &stage);

private:
    PlateDetector m_detector;
    CharacterClassifier m_classifier;
    bool m_initialized;
};

// 注册元类型，使 RecognitionResult 可以通过信号槽队列连接传递
// （跨线程的信号槽连接使用 Qt::QueuedConnection 时需要此声明）
Q_DECLARE_METATYPE(RecognitionResult)

#endif
'''

# ============================================================
# 6. PlateRecognizer.cpp
# ============================================================
plate_recognizer_cpp = '''\
#include "core/PlateRecognizer.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QFileInfo>
#include <QDebug>
#include <cmath>

// ------------------------------------------------------------------
// 构造函数
// 初始化检测器和分类器，加载字符模板
// Q_DECLARE_METATYPE 在头文件中完成编译时类型声明
// qRegisterMetaType 在此处完成运行时注册（跨线程队列连接必须）
// ------------------------------------------------------------------
PlateRecognizer::PlateRecognizer(QObject *parent)
    : QObject(parent), m_initialized(false) {

    // 运行时注册元类型，确保跨线程信号槽队列连接可以传递 RecognitionResult
    qRegisterMetaType<RecognitionResult>("RecognitionResult");

    // 加载字符模板（优先从 Qt 资源系统加载）
    // ":/templates" 是 Qt 资源文件中的路径前缀
    bool loaded = m_classifier.loadTemplates(":/templates");
    if (!loaded) {
        qWarning() << "Qt 资源模板加载失败，尝试从文件系统加载";
        m_classifier.loadTemplates("./templates");
    }

    m_initialized = true;
    qDebug() << "PlateRecognizer 初始化完成";
}

// ------------------------------------------------------------------
// 识别主流程
// 流程：检测车牌区域 -> 选择最佳候选 -> 字符分割与识别 -> 绘制标注
// 通过信号通知 UI 层当前进度
// ------------------------------------------------------------------
RecognitionResult PlateRecognizer::recognize(const cv::Mat &inputImage) {
    RecognitionResult result;

    if (!m_initialized) {
        result.errorMessage = "识别器未初始化";
        return result;
    }

    if (inputImage.empty()) {
        result.errorMessage = "输入图像为空";
        return result;
    }

    // 发出识别开始信号
    emit recognitionStarted();
    emit progressChanged(10, "检测车牌区域...");

    // Step 1: 检测所有候选车牌区域
    std::vector<PlateRegion> candidates = m_detector.detect(inputImage);
    result.candidates = candidates;

    if (candidates.empty()) {
        result.errorMessage = "未检测到车牌区域";
        emit progressChanged(100, "识别失败：未检测到车牌区域");
        emit recognitionFinished(result);
        return result;
    }

    emit progressChanged(50, "识别字符中...");

    // Step 2: 选择最佳候选区域
    // 策略：选择宽高比最接近中国标准车牌比例（440mm / 140mm = 3.14）的候选
    const double idealRatio = 3.14;
    int bestIdx = 0;
    double bestDiff = 999999.0;
    for (int i = 0; i < (int)candidates.size(); i++) {
        double diff = std::abs(candidates[i].aspectRatio - idealRatio);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx  = i;
        }
    }

    const PlateRegion &bestPlate = candidates[bestIdx];
    result.plateImage = bestPlate.plateImage.clone();

    // Step 3: 对最佳候选进行字符识别
    QString plateNumber = m_classifier.recognizePlate(bestPlate.plateGray);

    if (plateNumber.isEmpty()) {
        result.errorMessage = "字符识别失败";
        emit progressChanged(100, "识别失败：字符识别失败");
        emit recognitionFinished(result);
        return result;
    }

    result.plateNumber = plateNumber;
    result.success     = true;
    result.confidence  = 0.8;  // 默认置信度，后续可根据各字符匹配得分加权计算

    // Step 4: 在图像副本上绘制绿色矩形框，标注检测到的车牌位置
    result.processedImage = inputImage.clone();
    cv::rectangle(result.processedImage,
                  bestPlate.boundingBox,
                  cv::Scalar(0, 255, 0),  // 绿色边框（BGR 格式）
                  2);                      // 线宽 2 像素

    emit progressChanged(100, "识别完成");
    emit recognitionFinished(result);

    qDebug() << "识别完成:" << plateNumber;
    return result;
}

// ------------------------------------------------------------------
// 从文件路径加载图像并识别
// Windows 下使用 toStdWString() 处理宽字符路径，支持中文文件名和路径
// ------------------------------------------------------------------
RecognitionResult PlateRecognizer::recognizeFromFile(const QString &filePath) {
    RecognitionResult result;

    // 检查文件是否存在
    if (!QFileInfo::exists(filePath)) {
        result.errorMessage = QString("文件不存在: %1").arg(filePath);
        return result;
    }

    // Windows 下使用宽字符路径（toStdWString），支持中文路径和文件名
    // cv::imread 接受 std::wstring 参数（OpenCV 4.x Windows 特有问题）
    cv::Mat image = cv::imread(filePath.toStdWString());

    if (image.empty()) {
        result.errorMessage = QString("无法加载图像文件: %1").arg(filePath);
        return result;
    }

    return recognize(image);
}
'''


# ============================================================
# 写入所有文件
# ============================================================
if __name__ == "__main__":
    print(f"目标目录: {CORE_DIR}")
    print("-" * 60)

    write_file(os.path.join(CORE_DIR, "PlateDetector.h"),         plate_detector_h)
    write_file(os.path.join(CORE_DIR, "PlateDetector.cpp"),       plate_detector_cpp)
    write_file(os.path.join(CORE_DIR, "CharacterClassifier.h"),   character_classifier_h)
    write_file(os.path.join(CORE_DIR, "CharacterClassifier.cpp"), character_classifier_cpp)
    write_file(os.path.join(CORE_DIR, "PlateRecognizer.h"),       plate_recognizer_h)
    write_file(os.path.join(CORE_DIR, "PlateRecognizer.cpp"),     plate_recognizer_cpp)

    print("-" * 60)
    print("All 6 core files generated successfully!")
