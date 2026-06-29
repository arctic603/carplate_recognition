#include "utils/CsvExporter.h"
#include <QFile>
#include <QTextStream>

namespace CsvExporter {

bool exportToCsv(const QList<Record> &records, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    // \u5199\u5165 UTF-8 BOM
    stream << "\xEF\xBB\xBF";

    // \u5199\u5165\u8868\u5934
    stream << QString::fromUtf8(u8"\u5e8f\u53f7") << ","
           << QString::fromUtf8(u8"\u8f66\u724c\u53f7") << ","
           << QString::fromUtf8(u8"\u7f6e\u4fe1\u5ea6") << ","
           << QString::fromUtf8(u8"\u56fe\u7247\u8def\u5f84") << ","
           << QString::fromUtf8(u8"\u65f6\u95f4") << ","
           << QString::fromUtf8(u8"\u6765\u6e90") << ","
           << QString::fromUtf8(u8"\u7701\u4efd")
           << "\r\n";

    // \u5199\u5165\u6570\u636e\u884c
    int index = 1;
    for (const auto &r : records) {
        stream << index++ << ","
               << r.plateNumber << ","
               << QString::number(r.confidence, 'f', 4) << ","
               << r.imagePath << ","
               << r.timestamp.toString("yyyy-MM-dd hh:mm:ss") << ","
               << r.source << ","
               << r.province
               << "\r\n";
    }

    file.close();
    return true;
}

} // namespace CsvExporter
