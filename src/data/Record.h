#ifndef RECORD_H
#define RECORD_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QUuid>

/**
 * @brief \u8f66\u724c\u8bc6\u522b\u8bb0\u5f55\u6570\u636e\u7ed3\u6784\u4f53
 *
 * \u5305\u542b\u4e00\u6b21\u8f66\u724c\u8bc6\u522b\u7684\u6240\u6709\u4fe1\u606f\uff0c\u652f\u6301 JSON \u5e8f\u5217\u5316\u4e0e\u53cd\u5e8f\u5217\u5316
 */
struct Record {
    QString id;            // \u552f\u4e00\u6807\u8bc6 (UUID)
    QString plateNumber;   // \u8f66\u724c\u53f7
    double confidence;     // \u7f6e\u4fe1\u5ea6 (0~1)
    QString imagePath;     // \u56fe\u7247\u8def\u5f84
    QDateTime timestamp;   // \u8bc6\u522b\u65f6\u95f4
    QString source;        // \u6765\u6e90: "camera" \u6216 "file"
    QString province;      // \u7701\u4efd\u7b80\u79f0

    /**
     * @brief \u751f\u6210\u552f\u4e00 ID
     */
    static QString generateId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    /**
     * @brief \u5e8f\u5217\u5316\u4e3a JSON \u5bf9\u8c61
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["plateNumber"] = plateNumber;
        obj["confidence"] = confidence;
        obj["imagePath"] = imagePath;
        obj["timestamp"] = timestamp.toString(Qt::ISODate);
        obj["source"] = source;
        obj["province"] = province;
        return obj;
    }

    /**
     * @brief \u4ece JSON \u5bf9\u8c61\u53cd\u5e8f\u5217\u5316
     */
    static Record fromJson(const QJsonObject &obj) {
        Record r;
        r.id = obj["id"].toString();
        r.plateNumber = obj["plateNumber"].toString();
        r.confidence = obj["confidence"].toDouble();
        r.imagePath = obj["imagePath"].toString();
        r.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        r.source = obj["source"].toString();
        r.province = obj["province"].toString();
        return r;
    }
};

#endif // RECORD_H
