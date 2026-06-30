#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QDate>
#include <QSqlDatabase>
#include "data/Record.h"

class RecordManager : public QObject
{
    Q_OBJECT

public:
    explicit RecordManager(const QString &dbPath, QObject *parent = nullptr);
    ~RecordManager();

    // CRUD
    void addRecord(const Record &record);
    void deleteRecord(const QString &id);
    QList<Record> getAllRecords() const;
    void clearAll();

    // Search & filter
    QList<Record> searchByPlate(const QString &keyword) const;
    QList<Record> filterByDateRange(const QDateTime &from, const QDateTime &to) const;
    QList<Record> filterByProvince(const QString &province) const;

    // Statistics (SQL-side aggregation)
    QMap<QString, int> countByProvince() const;
    QMap<QDate, int> countByDate(const QDate &from, const QDate &to) const;
    int totalCount() const;
    double averageConfidence() const;

signals:
    void recordAdded(const Record &record);
    void recordsChanged();

private:
    bool openDatabase();
    void createTable();

    QSqlDatabase m_db;
    QString m_dbPath;
};

#endif // RECORDMANAGER_H
