#include "data/RecordManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>
#include <QVariant>

// ============================================================
// Record::generateId
// ============================================================
QString Record::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// ============================================================
// Constructor / Destructor
// ============================================================
RecordManager::RecordManager(const QString &dbPath, QObject *parent)
    : QObject(parent), m_dbPath(dbPath)
{
    if (openDatabase()) {
        createTable();
        qDebug() << "RecordManager: SQLite database opened:" << dbPath;
    } else {
        qCritical() << "RecordManager: failed to open database:" << dbPath;
    }
}

RecordManager::~RecordManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// ============================================================
// Database initialization
// ============================================================
bool RecordManager::openDatabase()
{
    // Ensure directory exists
    QDir dir = QFileInfo(m_dbPath).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Use unique connection name to avoid conflicts
    QString connName = "recordmanager_" + QString::number((quintptr)this);
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "SQLite open error:" << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode for better concurrent read performance
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode=WAL");
    pragma.exec("PRAGMA foreign_keys=ON");

    return true;
}

void RecordManager::createTable()
{
    QSqlQuery q(m_db);
    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS records ("
        "  id         TEXT PRIMARY KEY,"
        "  plate      TEXT NOT NULL,"
        "  confidence REAL NOT NULL DEFAULT 0.0,"
        "  image_path TEXT,"
        "  timestamp  TEXT NOT NULL,"
        "  source     TEXT,"
        "  province   TEXT"
        ")"
    );
    if (!ok) {
        qWarning() << "CREATE TABLE failed:" << q.lastError().text();
        return;
    }

    // Indexes for common queries
    q.exec("CREATE INDEX IF NOT EXISTS idx_records_plate ON records(plate)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_records_province ON records(province)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_records_timestamp ON records(timestamp)");
}

// ============================================================
// CRUD
// ============================================================
void RecordManager::addRecord(const Record &record)
{
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO records (id, plate, confidence, image_path, timestamp, source, province) "
        "VALUES (:id, :plate, :conf, :img, :ts, :src, :prov)"
    );
    q.bindValue(":id", record.id);
    q.bindValue(":plate", record.plateNumber);
    q.bindValue(":conf", record.confidence);
    q.bindValue(":img", record.imagePath);
    q.bindValue(":ts", record.timestamp.toString(Qt::ISODate));
    q.bindValue(":src", record.source);
    q.bindValue(":prov", record.province);

    if (!q.exec()) {
        qWarning() << "INSERT failed:" << q.lastError().text();
        return;
    }

    emit recordAdded(record);
}

void RecordManager::deleteRecord(const QString &id)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM records WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qWarning() << "DELETE failed:" << q.lastError().text();
        return;
    }

    emit recordsChanged();
}

QList<Record> RecordManager::getAllRecords() const
{
    QList<Record> list;
    QSqlQuery q(m_db);
    q.exec("SELECT id, plate, confidence, image_path, timestamp, source, province "
           "FROM records ORDER BY timestamp DESC");

    while (q.next()) {
        Record r;
        r.id           = q.value(0).toString();
        r.plateNumber  = q.value(1).toString();
        r.confidence   = q.value(2).toDouble();
        r.imagePath    = q.value(3).toString();
        r.timestamp    = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        r.source       = q.value(5).toString();
        r.province     = q.value(6).toString();
        list.append(r);
    }
    return list;
}

void RecordManager::clearAll()
{
    QSqlQuery q(m_db);
    q.exec("DELETE FROM records");
    emit recordsChanged();
}

// ============================================================
// Search & Filter
// ============================================================
QList<Record> RecordManager::searchByPlate(const QString &keyword) const
{
    QList<Record> list;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, plate, confidence, image_path, timestamp, source, province "
        "FROM records WHERE plate LIKE :kw ORDER BY timestamp DESC"
    );
    q.bindValue(":kw", "%" + keyword + "%");
    q.exec();

    while (q.next()) {
        Record r;
        r.id           = q.value(0).toString();
        r.plateNumber  = q.value(1).toString();
        r.confidence   = q.value(2).toDouble();
        r.imagePath    = q.value(3).toString();
        r.timestamp    = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        r.source       = q.value(5).toString();
        r.province     = q.value(6).toString();
        list.append(r);
    }
    return list;
}

QList<Record> RecordManager::filterByDateRange(const QDateTime &from, const QDateTime &to) const
{
    QList<Record> list;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, plate, confidence, image_path, timestamp, source, province "
        "FROM records WHERE timestamp >= :from AND timestamp <= :to "
        "ORDER BY timestamp DESC"
    );
    q.bindValue(":from", from.toString(Qt::ISODate));
    q.bindValue(":to", to.toString(Qt::ISODate));
    q.exec();

    while (q.next()) {
        Record r;
        r.id           = q.value(0).toString();
        r.plateNumber  = q.value(1).toString();
        r.confidence   = q.value(2).toDouble();
        r.imagePath    = q.value(3).toString();
        r.timestamp    = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        r.source       = q.value(5).toString();
        r.province     = q.value(6).toString();
        list.append(r);
    }
    return list;
}

QList<Record> RecordManager::filterByProvince(const QString &province) const
{
    QList<Record> list;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, plate, confidence, image_path, timestamp, source, province "
        "FROM records WHERE province = :prov ORDER BY timestamp DESC"
    );
    q.bindValue(":prov", province);
    q.exec();

    while (q.next()) {
        Record r;
        r.id           = q.value(0).toString();
        r.plateNumber  = q.value(1).toString();
        r.confidence   = q.value(2).toDouble();
        r.imagePath    = q.value(3).toString();
        r.timestamp    = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        r.source       = q.value(5).toString();
        r.province     = q.value(6).toString();
        list.append(r);
    }
    return list;
}

// ============================================================
// Statistics (SQL aggregation)
// ============================================================
QMap<QString, int> RecordManager::countByProvince() const
{
    QMap<QString, int> map;
    QSqlQuery q(m_db);
    q.exec("SELECT province, COUNT(*) FROM records "
           "WHERE province IS NOT NULL AND province != '' "
           "GROUP BY province ORDER BY COUNT(*) DESC");

    while (q.next()) {
        map[q.value(0).toString()] = q.value(1).toInt();
    }
    return map;
}

QMap<QDate, int> RecordManager::countByDate(const QDate &from, const QDate &to) const
{
    QMap<QDate, int> map;

    // Initialize all dates in range to 0
    QDate d = from;
    while (d <= to) {
        map[d] = 0;
        d = d.addDays(1);
    }

    // SQL aggregation using date substring
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT SUBSTR(timestamp, 1, 10) AS date_str, COUNT(*) "
        "FROM records "
        "WHERE timestamp >= :from AND timestamp <= :to "
        "GROUP BY date_str"
    );
    q.bindValue(":from", QDateTime(from, QTime(0, 0)).toString(Qt::ISODate));
    q.bindValue(":to", QDateTime(to, QTime(23, 59, 59)).toString(Qt::ISODate));
    q.exec();

    while (q.next()) {
        QDate date = QDate::fromString(q.value(0).toString(), "yyyy-MM-dd");
        if (date.isValid()) {
            map[date] = q.value(1).toInt();
        }
    }
    return map;
}

int RecordManager::totalCount() const
{
    QSqlQuery q(m_db);
    q.exec("SELECT COUNT(*) FROM records");
    if (q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

double RecordManager::averageConfidence() const
{
    QSqlQuery q(m_db);
    q.exec("SELECT AVG(confidence) FROM records");
    if (q.next()) {
        return q.value(0).toDouble();
    }
    return 0.0;
}
