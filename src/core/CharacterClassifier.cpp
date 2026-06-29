#include "core/CharacterClassifier.h"
#include <algorithm>
#include <QDebug>

CharacterClassifier::CharacterClassifier() {}

bool CharacterClassifier::loadTemplates(const QString &templateDir) {
    QDir baseDir(templateDir);
    if (!baseDir.exists()) {
        qWarning() << "Template dir not found:" << templateDir << "- using builtin";
        generateBuiltinTemplates();
        return false;
    }
    loadTemplatesFromDir(baseDir.filePath("provinces"), m_provinceTemplates);
    loadTemplatesFromDir(baseDir.filePath("letters"), m_letterTemplates);
    loadTemplatesFromDir(baseDir.filePath("alphanum"), m_alphanumTemplates);
    if (m_letterTemplates.empty() || m_alphanumTemplates.empty()) {
        qWarning() << "Template sets empty, generating builtin";
        generateBuiltinTemplates();
    }
    return true;
}

void CharacterClassifier::loadTemplatesFromDir(const QString &dir,
        std::map<QString, cv::Mat> &templates) {
    QDir templateDir(dir);
    if (!templateDir.exists()) return;
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.bmp";
    QStringList files = templateDir.entryList(filters, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        QString fullPath = templateDir.filePath(file);
        cv::Mat img = cv::imread(fullPath.toUtf8().constData(), cv::IMREAD_GRAYSCALE);
        if (img.empty()) { qWarning() << "Cannot load template:" << fullPath; continue; }
        cv::Mat binary;
        cv::threshold(img, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QFileInfo(file).baseName();
        templates[key] = binary;
    }
    qDebug() << "Loaded" << (int)templates.size() << "templates from" << dir;
}

void CharacterClassifier::generateBuiltinTemplates() {
    qDebug() << "Generating builtin character templates...";
    for (int i = 0; i < 26; i++) {
        cv::Mat canvas(40, 20, CV_8UC1, cv::Scalar(255));
        char ch = (char)('A' + i);
        std::string text(1, ch);
        cv::putText(canvas, text, cv::Point(2, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0), 2);
        cv::Mat binary;
        cv::threshold(canvas, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QString(QChar(ch));
        m_letterTemplates[key] = binary;
        m_alphanumTemplates[key] = binary;
    }
    for (int i = 0; i <= 9; i++) {
        cv::Mat canvas(40, 20, CV_8UC1, cv::Scalar(255));
        char ch = (char)('0' + i);
        std::string text(1, ch);
        cv::putText(canvas, text, cv::Point(2, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0), 2);
        cv::Mat binary;
        cv::threshold(canvas, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QString(QChar(ch));
        m_alphanumTemplates[key] = binary;
    }
    for (int i = 0; i <= 9; i++) {
        cv::Mat canvas(40, 20, CV_8UC1, cv::Scalar(255));
        cv::rectangle(canvas, cv::Point(2, 2), cv::Point(17, 37), cv::Scalar(0), -1);
        cv::Mat binary;
        cv::threshold(canvas, binary, 128, 255, cv::THRESH_BINARY_INV);
        QString key = QString("P%1").arg(i);
        m_provinceTemplates[key] = binary;
    }
    qDebug() << "Builtin templates:" << (int)m_provinceTemplates.size()
             << "provinces," << (int)m_letterTemplates.size() << "letters,"
             << (int)m_alphanumTemplates.size() << "alphanum";
}

std::vector<cv::Mat> CharacterClassifier::segmentCharacters(const cv::Mat &plateGray) {
    std::vector<cv::Mat> chars;
    if (plateGray.empty()) return chars;

    int h = plateGray.rows;
    int w = plateGray.cols;
    if (h < 10 || w < 10) return chars;

    // Resize to standard height 140px for consistent processing
    int targetH = 140;
    double ratio = (double)targetH / h;
    int newW = (int)(w * ratio);
    cv::Mat resized;
    cv::resize(plateGray, resized, cv::Size(newW, targetH), 0, 0, cv::INTER_CUBIC);

    // CLAHE for contrast enhancement
    cv::Mat enhanced;
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8, 8));
    clahe->apply(resized, enhanced);

    // Otsu threshold (characters are bright on dark plate background)
    cv::Mat binary;
    cv::threshold(enhanced, binary, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);

    // Find text band using horizontal projection
    // Characters occupy a band in the upper portion; border/noise below
    int gapThreshold = (int)(newW * 0.02);
    int textBottom = (int)(targetH * 0.65);  // default
    for (int row = (int)(targetH * 0.15); row < (int)(targetH * 0.65); row++) {
        int whiteCount = cv::countNonZero(binary.row(row));
        if (whiteCount < gapThreshold) {
            textBottom = row;
            break;
        }
    }

    // Extract text band from binary
    cv::Mat textBand = binary(cv::Rect(0, 0, newW, textBottom)).clone();
    int bandH = textBand.rows;
    if (bandH < 10) return chars;

    // Light morphological open to remove noise
    cv::Mat kOpen = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 1));
    cv::morphologyEx(textBand, textBand, cv::MORPH_OPEN, kOpen);

    // Dilate horizontally to merge character strokes
    cv::Mat kDilate = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 2));
    cv::Mat merged;
    cv::dilate(textBand, merged, kDilate);

    // Find contours in merged image
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(merged, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Collect valid character candidates
    struct CharCandidate {
        int x, y, w, h;
        cv::Mat img;
    };
    std::vector<CharCandidate> candidates;

    for (const auto &cnt : contours) {
        cv::Rect rect = cv::boundingRect(cnt);
        int area = rect.width * rect.height;
        double aspect = (double)rect.width / std::max(1, rect.height);
        double hRatio = (double)rect.height / bandH;

        // Filter: reject tiny noise, huge blobs, wrong proportions
        if (area < 80) continue;
        if (area > bandH * newW * 0.2) continue;
        if (aspect < 0.15 || aspect > 1.5) continue;
        if (hRatio < 0.3) continue;
        if (rect.height < bandH * 0.35) continue;

        // Extract from ORIGINAL textBand (not dilated)
        int pad = 2;
        int ry1 = std::max(0, rect.y - pad);
        int ry2 = std::min(textBand.rows, rect.y + rect.height + pad);
        int rx1 = std::max(0, rect.x - pad);
        int rx2 = std::min(textBand.cols, rect.x + rect.width + pad);
        cv::Mat charImg = textBand(cv::Rect(rx1, ry1, rx2-rx1, ry2-ry1)).clone();
        candidates.push_back({rect.x, rect.y, rect.width, rect.height, charImg});
    }

    // Sort by x position
    std::sort(candidates.begin(), candidates.end(),
        [](const CharCandidate &a, const CharCandidate &b) { return a.x < b.x; });

    // Extract character images
    for (const auto &c : candidates) {
        chars.push_back(c.img);
    }

    if ((int)chars.size() != 7) {
        qWarning() << "Character count:" << (int)chars.size() << "(expected 7)";
    }

    return chars;
}

cv::Mat CharacterClassifier::normalizeChar(const cv::Mat &charImg,
        int targetWidth, int targetHeight) {
    cv::Mat normalized;
    cv::resize(charImg, normalized, cv::Size(targetWidth, targetHeight));
    return normalized;
}

CharacterResult CharacterClassifier::matchCharacter(
        const cv::Mat &charImg,
        const std::map<QString, cv::Mat> &templates) {
    CharacterResult best;
    best.confidence = -1.0;
    best.character = "?";
    cv::Mat normalized = normalizeChar(charImg);
    for (const auto &entry : templates) {
        const cv::Mat &tmpl = entry.second;
        if (tmpl.empty()) continue;
        if (normalized.size() != tmpl.size()) continue;
        cv::Mat result;
        cv::matchTemplate(normalized, tmpl, result, cv::TM_CCOEFF_NORMED);
        double score = result.at<float>(0, 0);
        if (score > best.confidence) {
            best.confidence = score;
            best.character = entry.first;
        }
    }
    if (best.confidence >= -1.0) {
        best.confidence = (best.confidence + 1.0) / 2.0;
    } else {
        best.confidence = 0.0;
        best.character = "?";
    }
    return best;
}

std::vector<CharacterResult> CharacterClassifier::classify(const cv::Mat &plateGray) {
    std::vector<CharacterResult> results;
    std::vector<cv::Mat> chars = segmentCharacters(plateGray);
    for (int i = 0; i < (int)chars.size(); i++) {
        CharacterResult result;
        if (i == 0) {
            result = matchCharacter(chars[i], m_provinceTemplates);
        } else if (i == 1) {
            result = matchCharacter(chars[i], m_letterTemplates);
        } else {
            result = matchCharacter(chars[i], m_alphanumTemplates);
        }
        results.push_back(result);
    }
    return results;
}

QString CharacterClassifier::recognizePlate(const cv::Mat &plateGray) {
    std::vector<cv::Mat> chars = segmentCharacters(plateGray);
    if (chars.empty()) {
        qWarning() << "No characters segmented";
        return QString();
    }
    QString plateNumber;
    for (int i = 0; i < (int)chars.size(); i++) {
        CharacterResult result;
        if (i == 0) {
            result = matchCharacter(chars[i], m_provinceTemplates);
            if (result.confidence < 0.3) {
                CharacterResult alt = matchCharacter(chars[i], m_letterTemplates);
                if (alt.confidence > result.confidence) result = alt;
            }
        } else if (i == 1) {
            result = matchCharacter(chars[i], m_letterTemplates);
        } else {
            result = matchCharacter(chars[i], m_alphanumTemplates);
        }
        if (result.character.isEmpty() || result.character == "?") {
            plateNumber += "?";
        } else {
            plateNumber += result.character;
        }
    }
    return plateNumber;
}