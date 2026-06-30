#ifndef RECORD_H
#define RECORD_H

#include <QString>
#include <QDateTime>

struct Record {
    QString id;
    QString plateNumber;
    double confidence = 0.0;
    QString imagePath;
    QDateTime timestamp;
    QString source;     // "camera" or "file"
    QString province;

    static QString generateId();
};

#endif // RECORD_H
