#include "HistoryWidget.h"
#include "../data/RecordManager.h"
#include "../utils/ImageConverter.h"
#include "../utils/CsvExporter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QDate>

// -------------------------------------------------------
// 构造
// -------------------------------------------------------
HistoryWidget::HistoryWidget(RecordManager *recordManager, QWidget *parent)
    : QWidget(parent)
    , m_recordManager(recordManager)
{
    setupUI();

    // 记录变更时自动刷新
    connect(m_recordManager, &RecordManager::recordsChanged,
            this, &HistoryWidget::refreshTable);
}

// -------------------------------------------------------
// 界面构建
// -------------------------------------------------------
void HistoryWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ===== 顶部搜索栏 =====
    QHBoxLayout *filterLayout = new QHBoxLayout();

    // 搜索输入
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText(QStringLiteral("搜索车牌号..."));
    m_searchInput->setClearButtonEnabled(true);
    filterLayout->addWidget(new QLabel(QStringLiteral("车牌号:")));
    filterLayout->addWidget(m_searchInput);

    // 日期范围
    m_dateFrom = new QDateEdit(QDate::currentDate().addYears(-1), this);
    m_dateFrom->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateFrom->setCalendarPopup(true);
    m_dateTo = new QDateEdit(QDate::currentDate(), this);
    m_dateTo->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateTo->setCalendarPopup(true);

    filterLayout->addWidget(new QLabel(QStringLiteral("从:")));
    filterLayout->addWidget(m_dateFrom);
    filterLayout->addWidget(new QLabel(QStringLiteral("到:")));
    filterLayout->addWidget(m_dateTo);

    // 省份过滤
    m_provinceFilter = new QComboBox(this);
    m_provinceFilter->addItem(QStringLiteral("全部省份"));
    filterLayout->addWidget(new QLabel(QStringLiteral("省份:")));
    filterLayout->addWidget(m_provinceFilter);

    mainLayout->addLayout(filterLayout);

    // ===== 操作按钮行 =====
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnSearch  = new QPushButton(QStringLiteral("搜索"), this);
    m_btnRefresh = new QPushButton(QStringLiteral("刷新"), this);
    m_btnDelete  = new QPushButton(QStringLiteral("删除选中"), this);
    m_btnExport  = new QPushButton(QStringLiteral("导出CSV"), this);

    m_btnSearch->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #1976D2; color: white; padding: 6px 16px; }"));
    m_btnDelete->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #E53935; color: white; padding: 6px 16px; }"));
    m_btnExport->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #388E3C; color: white; padding: 6px 16px; }"));

    btnLayout->addWidget(m_btnSearch);
    btnLayout->addWidget(m_btnRefresh);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addWidget(m_btnExport);
    mainLayout->addLayout(btnLayout);

    // ===== 数据表格 =====
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("序号"),
        QStringLiteral("车牌号"),
        QStringLiteral("置信度"),
        QStringLiteral("来源"),
        QStringLiteral("时间"),
        QStringLiteral("操作")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 60);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table, 1);

    // ===== 信号连接 =====
    connect(m_searchInput, &QLineEdit::textChanged,
            this, &HistoryWidget::onSearchTextChanged);
    connect(m_dateFrom, &QDateEdit::dateChanged,
            this, &HistoryWidget::onDateFilterChanged);
    connect(m_dateTo, &QDateEdit::dateChanged,
            this, &HistoryWidget::onDateFilterChanged);
    connect(m_provinceFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HistoryWidget::onProvinceFilterChanged);
    connect(m_btnSearch,  &QPushButton::clicked, this, &HistoryWidget::refreshTable);
    connect(m_btnRefresh, &QPushButton::clicked, this, &HistoryWidget::refreshTable);
    connect(m_btnDelete,  &QPushButton::clicked, this, &HistoryWidget::onDeleteSelected);
    connect(m_btnExport,  &QPushButton::clicked, this, &HistoryWidget::onExportCsv);
}

// -------------------------------------------------------
// 刷新表格
// -------------------------------------------------------
void HistoryWidget::refreshTable()
{
    // 先获取基础记录集
    QList<Record> records;
    QString keyword = m_searchInput->text().trimmed();
    if (!keyword.isEmpty()) {
        records = m_recordManager->searchByPlate(keyword);
    } else {
        records = m_recordManager->getAllRecords();
    }

    // 日期过滤
    QDateTime from = m_dateFrom->dateTime();
    QDateTime to   = m_dateTo->dateTime().addSecs(86399); // 包含整天
    QList<Record> dateFiltered;
    for (const Record &r : records) {
        if (r.timestamp >= from && r.timestamp <= to)
            dateFiltered.append(r);
    }
    records = dateFiltered;

    // 省份过滤
    QString provinceText = m_provinceFilter->currentText();
    if (!provinceText.startsWith(QStringLiteral("全部省份"))) {
        // 提取省份名（去掉括号中的数量）
        QString province = provinceText.split(QStringLiteral(" ")).first();
        QList<Record> provFiltered;
        for (const Record &r : records) {
            if (r.province == province)
                provFiltered.append(r);
        }
        records = provFiltered;
    }

    m_currentRecords = records;

    // 填充表格
    m_table->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const Record &r = records.at(i);

        // 序号
        QTableWidgetItem *noItem = new QTableWidgetItem(QString::number(i + 1));
        noItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, noItem);

        // 车牌号
        m_table->setItem(i, 1, new QTableWidgetItem(r.plateNumber));

        // 置信度（带颜色标记）
        double pct = r.confidence * 100.0;
        QTableWidgetItem *confItem = new QTableWidgetItem(
            QStringLiteral("%1%").arg(pct, 0, 'f', 1));
        confItem->setTextAlignment(Qt::AlignCenter);
        if (pct > 80.0)
            confItem->setForeground(QColor(0x4C, 0xAF, 0x50)); // 绿色
        else if (pct > 50.0)
            confItem->setForeground(QColor(0xFF, 0x98, 0x00)); // 黄色
        else
            confItem->setForeground(QColor(0xF4, 0x43, 0x36)); // 红色
        m_table->setItem(i, 2, confItem);

        // 来源
        m_table->setItem(i, 3, new QTableWidgetItem(r.source));

        // 时间
        m_table->setItem(i, 4, new QTableWidgetItem(
            r.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));

        // 操作列按钮
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 2, 4, 2);
        actionLayout->setSpacing(4);

        QPushButton *viewBtn   = new QPushButton(QStringLiteral("查看"));
        QPushButton *deleteBtn = new QPushButton(QStringLiteral("删除"));
        viewBtn->setFixedSize(50, 26);
        deleteBtn->setFixedSize(50, 26);

        // 使用 recordId 绑定，避免行号失效
        QString recId = r.id;
        connect(viewBtn, &QPushButton::clicked, this, [this, recId]() {
            onViewDetail(recId);
        });
        connect(deleteBtn, &QPushButton::clicked, this, [this, recId]() {
            onDeleteRecord(recId);
        });

        actionLayout->addWidget(viewBtn);
        actionLayout->addWidget(deleteBtn);
        actionLayout->addStretch();
        m_table->setCellWidget(i, 5, actionWidget);
    }

    // 更新省份过滤器
    populateProvinceFilter();
}

// -------------------------------------------------------
// 搜索/过滤变化
// -------------------------------------------------------
void HistoryWidget::onSearchTextChanged()
{
    refreshTable();
}

void HistoryWidget::onDateFilterChanged()
{
    refreshTable();
}

void HistoryWidget::onProvinceFilterChanged()
{
    refreshTable();
}

// -------------------------------------------------------
// 删除选中行
// -------------------------------------------------------
void HistoryWidget::onDeleteSelected()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_currentRecords.size()) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在表格中选择要删除的记录。"));
        return;
    }

    const Record &r = m_currentRecords.at(row);
    QMessageBox::StandardButton btn = QMessageBox::question(
        this,
        QStringLiteral("确认删除"),
        QStringLiteral("确定要删除车牌号 '%1' 的识别记录吗？").arg(r.plateNumber),
        QMessageBox::Yes | QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        m_recordManager->deleteRecord(r.id);
        refreshTable();
    }
}

// -------------------------------------------------------
// 删除指定记录
// -------------------------------------------------------
void HistoryWidget::onDeleteRecord(const QString &recordId)
{
    QMessageBox::StandardButton btn = QMessageBox::question(
        this,
        QStringLiteral("确认删除"),
        QStringLiteral("确定要删除此条识别记录吗？"),
        QMessageBox::Yes | QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        m_recordManager->deleteRecord(recordId);
        refreshTable();
    }
}

// -------------------------------------------------------
// 导出 CSV
// -------------------------------------------------------
void HistoryWidget::onExportCsv()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出CSV文件"),
        QString(),
        QStringLiteral("CSV 文件 (*.csv)"));
    if (filePath.isEmpty())
        return;

    // 导出当前过滤后的记录（若无过滤则导出全部）
    QList<Record> records = m_currentRecords.isEmpty()
                                ? m_recordManager->getAllRecords()
                                : m_currentRecords;

    // 调用 CsvExporter 工具导出
    bool ok = CsvExporter::exportToCsv(records, filePath);

    if (ok) {
        QMessageBox::information(
            this,
            QStringLiteral("导出成功"),
            QStringLiteral("成功导出 %1 条记录到:\n%2")
                .arg(records.size())
                .arg(filePath));
    } else {
        QMessageBox::warning(
            this,
            QStringLiteral("导出失败"),
            QStringLiteral("CSV 文件导出失败，请检查文件路径和权限。"));
    }
}

// -------------------------------------------------------
// 查看详情对话框
// -------------------------------------------------------
void HistoryWidget::onViewDetail(const QString &recordId)
{
    // 从所有记录中查找目标记录
    QList<Record> allRecords = m_recordManager->getAllRecords();
    Record record;
    bool found = false;
    for (const Record &r : allRecords) {
        if (r.id == recordId) {
            record = r;
            found = true;
            break;
        }
    }
    if (!found) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("记录未找到，可能已被删除。"));
        return;
    }

    // 构建详情对话框
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("识别详情 - %1").arg(record.plateNumber));
    dialog->resize(700, 550);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // --- 图片区域 ---
    QHBoxLayout *imageLayout = new QHBoxLayout();

    // 原图
    QVBoxLayout *origLayout = new QVBoxLayout();
    origLayout->addWidget(new QLabel(QStringLiteral("原始图片:")));
    QLabel *origLabel = new QLabel();
    QPixmap origPixmap(record.imagePath);
    if (!origPixmap.isNull()) {
        origLabel->setPixmap(origPixmap.scaled(320, 240, Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation));
    } else {
        origLabel->setText(QStringLiteral("图片不可用"));
    }
    origLabel->setAlignment(Qt::AlignCenter);
    origLayout->addWidget(origLabel);
    imageLayout->addLayout(origLayout);

    // 车牌裁剪图
    QVBoxLayout *plateLayout = new QVBoxLayout();
    plateLayout->addWidget(new QLabel(QStringLiteral("车牌裁剪图:")));
    QLabel *plateLabel = new QLabel();
    // 车牌图路径 = 原图路径去掉扩展名 + "_plate.jpg"
    QString platePath = record.imagePath;
    int dotPos = platePath.lastIndexOf('.');
    if (dotPos > 0)
        platePath = platePath.left(dotPos) + "_plate.jpg";
    QPixmap platePixmap(platePath);
    if (!platePixmap.isNull()) {
        plateLabel->setPixmap(platePixmap.scaled(200, 80, Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
    } else {
        plateLabel->setText(QStringLiteral("无车牌图"));
    }
    plateLabel->setAlignment(Qt::AlignCenter);
    plateLayout->addWidget(plateLabel);
    imageLayout->addLayout(plateLayout);

    mainLayout->addLayout(imageLayout);

    // --- 详细信息 ---
    QLabel *infoLabel = new QLabel(dialog);
    infoLabel->setText(QStringLiteral(
        "<table style='font-size:14px;'>"
        "<tr><td><b>车牌号:</b></td><td>%1</td></tr>"
        "<tr><td><b>置信度:</b></td><td>%2%</td></tr>"
        "<tr><td><b>省份:</b></td><td>%3</td></tr>"
        "<tr><td><b>来源:</b></td><td>%4</td></tr>"
        "<tr><td><b>时间:</b></td><td>%5</td></tr>"
        "<tr><td><b>记录ID:</b></td><td>%6</td></tr>"
        "</table>")
        .arg(record.plateNumber)
        .arg(record.confidence * 100, 0, 'f', 1)
        .arg(record.province)
        .arg(record.source)
        .arg(record.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
        .arg(record.id));
    mainLayout->addWidget(infoLabel);

    mainLayout->addStretch();

    // 关闭按钮
    QPushButton *closeBtn = new QPushButton(QStringLiteral("关闭"), dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    QHBoxLayout *closeLayout = new QHBoxLayout();
    closeLayout->addStretch();
    closeLayout->addWidget(closeBtn);
    mainLayout->addLayout(closeLayout);

    dialog->exec();
    delete dialog;
}

// -------------------------------------------------------
// 填充省份下拉框
// -------------------------------------------------------
void HistoryWidget::populateProvinceFilter()
{
    // 保存当前选择
    QString currentText = m_provinceFilter->currentText();

    m_provinceFilter->blockSignals(true);
    m_provinceFilter->clear();
    m_provinceFilter->addItem(QStringLiteral("全部省份"));

    QMap<QString, int> counts = m_recordManager->countByProvince();
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        m_provinceFilter->addItem(
            QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));
    }

    // 恢复之前的选择
    int idx = m_provinceFilter->findText(currentText);
    if (idx >= 0)
        m_provinceFilter->setCurrentIndex(idx);

    m_provinceFilter->blockSignals(false);
}
