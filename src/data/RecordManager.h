#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QDate>
#include "data/Record.h"

/**
 * @brief \u8bc6\u522b\u8bb0\u5f55\u7ba1\u7406\u5668
 *
 * \u8d1f\u8d23\u8bb0\u5f55\u7684\u589e\u5220\u6539\u67e5\u3001\u7edf\u8ba1\u5206\u6790\u4ee5\u53ca JSON \u6301\u4e45\u5316\u5b58\u50a8\u3002
 * \u4f7f\u7528 QSaveFile \u8fdb\u884c\u539f\u5b50\u5199\u5165\uff0c\u907f\u514d\u6570\u636e\u635f\u574f\u3002
 */
class RecordManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief \u6784\u9020\u51fd\u6570
     * @param filePath JSON \u6570\u636e\u6587\u4ef6\u8def\u5f84
     * @param parent \u7236\u5bf9\u8c61
     */
    explicit RecordManager(const QString &filePath, QObject *parent = nullptr);

    // ---- \u57fa\u672c\u64cd\u4f5c ----
    void addRecord(const Record &record);
    void deleteRecord(const QString &id);
    QList<Record> getAllRecords() const;
    void clearAll();

    // ---- \u641c\u7d22\u4e0e\u8fc7\u6ee4 ----
    QList<Record> searchByPlate(const QString &keyword) const;
    QList<Record> filterByDateRange(const QDateTime &from, const QDateTime &to) const;
    QList<Record> filterByProvince(const QString &province) const;

    // ---- \u7edf\u8ba1 ----
    QMap<QString, int> countByProvince() const;
    QMap<QDate, int> countByDate(const QDate &from, const QDate &to) const;
    int totalCount() const;
    double averageConfidence() const;

signals:
    /**
     * @brief \u65b0\u8bb0\u5f55\u5df2\u6dfb\u52a0
     */
    void recordAdded(const Record &record);

    /**
     * @brief \u8bb0\u5f55\u5217\u8868\u5df2\u53d8\u66f4\uff08\u5220\u9664\u3001\u6e05\u7a7a\u7b49\u64cd\u4f5c\u540e\u89e6\u53d1\uff09
     */
    void recordsChanged();

private:
    void loadFromFile();
    void saveToFile() const;

    QString m_filePath;
    QList<Record> m_records;
};

#endif // RECORDMANAGER_H
