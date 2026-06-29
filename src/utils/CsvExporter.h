#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <QString>
#include <QList>
#include "data/Record.h"

/**
 * @brief CSV \u5bfc\u51fa\u5de5\u5177\u547d\u540d\u7a7a\u95f4
 *
 * \u5c06\u8bc6\u522b\u8bb0\u5f55\u5bfc\u51fa\u4e3a UTF-8 BOM \u7f16\u7801\u7684 CSV \u6587\u4ef6
 */
namespace CsvExporter {

/**
 * @brief \u5c06\u8bb0\u5f55\u5217\u8868\u5bfc\u51fa\u5230 CSV \u6587\u4ef6
 * @param records \u5f85\u5bfc\u51fa\u7684\u8bb0\u5f55\u5217\u8868
 * @param filePath \u76ee\u6807\u6587\u4ef6\u8def\u5f84
 * @return \u5bfc\u51fa\u662f\u5426\u6210\u529f
 */
bool exportToCsv(const QList<Record> &records, const QString &filePath);

} // namespace CsvExporter

#endif // CSVEXPORTER_H
