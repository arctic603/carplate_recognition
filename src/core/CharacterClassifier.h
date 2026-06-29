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
