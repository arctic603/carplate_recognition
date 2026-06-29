#include "data/RecordManager.h"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

RecordManager::RecordManager(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
    loadFromFile();
}

void RecordManager::addRecord(const Record &record)
{
    m_records.prepend(record);
    saveToFile();
    emit recordAdded(record);
}

void RecordManager::deleteRecord(const QString &id)
{
    m_records.erase(
        std::remove_if(m_records.begin(), m_records.end(),
                        [&id](const Record &r) { return r.id == id; }),
        m_records.end());
    saveToFile();
    emit recordsChanged();
}

QList<Record> RecordManager::getAllRecords() const
{
    return m_records;
}

void RecordManager::clearAll()
{
    m_records.clear();
    saveToFile();
    emit recordsChanged();
}

// ---- \u641c\u7d22\u4e0e\u8fc7\u6ee4 ----

QList<Record> RecordManager::searchByPlate(const QString &keyword) const
{
    QList<Record> result;
    for (const auto &r : m_records) {
        if (r.plateNumber.contains(keyword, Qt::CaseInsensitive)) {
            result.append(r);
        }
    }
    return result;
}

QList<Record> RecordManager::filterByDateRange(const QDateTime &from, const QDateTime &to) const
{
    QList<Record> result;
    for (const auto &r : m_records) {
        if (r.timestamp >= from && r.timestamp <= to) {
            result.append(r);
        }
    }
    return result;
}

QList<Record> RecordManager::filterByProvince(const QString &province) const
{
    QList<Record> result;
    for (const auto &r : m_records) {
        if (r.province == province) {
            result.append(r);
        }
    }
    return result;
}

// ---- \u7edf\u8ba1 ----

QMap<QString, int> RecordManager::countByProvince() const
{
    QMap<QString, int> map;
    for (const auto &r : m_records) {
        if (!r.province.isEmpty()) {
            map[r.province]++;
        }
    }
    return map;
}

QMap<QDate, int> RecordManager::countByDate(const QDate &from, const QDate &to) const
{
    QMap<QDate, int> map;
    // \u521d\u59cb\u5316\u65e5\u671f\u8303\u56f4\u5185\u6240\u6709\u65e5\u671f\u4e3a 0
    QDate d = from;
    while (d <= to) {
        map[d] = 0;
        d = d.addDays(1);
    }
    // \u7edf\u8ba1\u6bcf\u65e5\u8bc6\u522b\u6570\u91cf
    for (const auto &r : m_records) {
        QDate date = r.timestamp.date();
        if (date >= from && date <= to) {
            map[date]++;
        }
    }
    return map;
}

int RecordManager::totalCount() const
{
    return m_records.size();
}

double RecordManager::averageConfidence() const
{
    if (m_records.isEmpty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (const auto &r : m_records) {
        sum += r.confidence;
    }
    return sum / m_records.size();
}

// ---- \u79c1\u6709\u65b9\u6cd5 ----

void RecordManager::loadFromFile()
{
    QFile file(m_filePath);
    if (!file.exists()) {
        // \u6587\u4ef6\u4e0d\u5b58\u5728\u65f6\u8fd4\u56de\u7a7a\u5217\u8868
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray arr = root["records"].toArray();

    m_records.clear();
    m_records.reserve(arr.size());
    for (const auto &val : arr) {
        m_records.append(Record::fromJson(val.toObject()));
    }
}

void RecordManager::saveToFile() const
{
    // \u786e\u4fdd\u76ee\u5f55\u5b58\u5728
    QDir dir = QFileInfo(m_filePath).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // \u6784\u5efa JSON \u6570\u636e
    QJsonArray arr;
    for (const auto &r : m_records) {
        arr.append(r.toJson());
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["records"] = arr;

    // \u4f7f\u7528 QSaveFile \u539f\u5b50\u5199\u5165\uff0c\u907f\u514d\u5199\u5165\u4e2d\u65ad\u5bfc\u81f4\u6570\u636e\u635f\u574f
    QSaveFile saveFile(m_filePath);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        return;
    }

    saveFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    saveFile.commit();
}
