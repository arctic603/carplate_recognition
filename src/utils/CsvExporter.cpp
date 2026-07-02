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

    // 写入 UTF-8 BOM
    stream << "\xEF\xBB\xBF";

    // 写入表头
    stream << QString::fromUtf8(u8"序号") << ","
           << QString::fromUtf8(u8"车牌号") << ","
           << QString::fromUtf8(u8"置信度") << ","
           << QString::fromUtf8(u8"图片路径") << ","
           << QString::fromUtf8(u8"时间") << ","
           << QString::fromUtf8(u8"来源") << ","
           << QString::fromUtf8(u8"省份")
           << "\r\n";

    // 写入数据行
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
