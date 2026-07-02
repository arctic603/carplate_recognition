#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <QString>
#include <QList>
#include "data/Record.h"

/**
 * @brief CSV 导出工具命名空间
 *
 * 将识别记录导出为 UTF-8 BOM 编码的 CSV 文件
 */
namespace CsvExporter {

/**
 * @brief 将记录列表导出到 CSV 文件
 * @param records 待导出的记录列表
 * @param filePath 目标文件路径
 * @return 导出是否成功
 */
bool exportToCsv(const QList<Record> &records, const QString &filePath);

} // namespace CsvExporter

#endif // CSVEXPORTER_H
