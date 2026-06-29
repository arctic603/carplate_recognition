#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QWidget>
#include "../data/Record.h"

class QLineEdit;
class QDateEdit;
class QComboBox;
class QTableWidget;
class QPushButton;
class RecordManager;

/**
 * @brief 识别记录管理页面
 *
 * 提供按车牌号搜索、按日期/省份过滤、查看删除记录、
 * 导出 CSV 等功能。
 */
class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryWidget(RecordManager *recordManager, QWidget *parent = nullptr);

public slots:
    /** 刷新表格（综合所有过滤条件） */
    void refreshTable();
    /** 导出 CSV */
    void onExportCsv();

private slots:
    /** 搜索文本变化 */
    void onSearchTextChanged();
    /** 日期过滤条件变化 */
    void onDateFilterChanged();
    /** 省份过滤条件变化 */
    void onProvinceFilterChanged();
    /** 删除选中行 */
    void onDeleteSelected();
    /** 查看指定记录的详情 */
    void onViewDetail(const QString &recordId);
    /** 删除指定记录 */
    void onDeleteRecord(const QString &recordId);

private:
    /** 构建界面 */
    void setupUI();
    /** 填充省份过滤下拉框 */
    void populateProvinceFilter();

    RecordManager *m_recordManager = nullptr;

    // --- 搜索/过滤组件 ---
    QLineEdit   *m_searchInput    = nullptr;
    QDateEdit   *m_dateFrom       = nullptr;
    QDateEdit   *m_dateTo         = nullptr;
    QComboBox   *m_provinceFilter = nullptr;
    QPushButton *m_btnSearch      = nullptr;
    QPushButton *m_btnRefresh     = nullptr;
    QPushButton *m_btnDelete      = nullptr;
    QPushButton *m_btnExport      = nullptr;

    // --- 数据表格 ---
    QTableWidget *m_table = nullptr;

    // --- 当前显示的记录列表 ---
    QList<Record> m_currentRecords;
};

#endif // HISTORYWIDGET_H
