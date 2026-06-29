#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QDate>
#include <QMap>
#include <QLabel>
#include <QDateEdit>
#include <QPushButton>

class RecordManager;

// 自定义饼图控件（使用 QPainter 绘制）
class PieChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit PieChartWidget(QWidget *parent = nullptr);
    void setData(const QMap<QString, int> &data);  // 省份 -> 数量

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QString, int> m_data;
    QList<QColor> m_colors;
    void initColors();
};

// 自定义柱状图控件（使用 QPainter 绘制）
class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    void setData(const QMap<QDate, int> &data);  // 日期 -> 数量

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QDate, int> m_data;
    QList<QPair<QString, int>> m_bars;  // 标签 + 值
};

// 统计页面主控件
class StatisticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatisticsWidget(RecordManager *recordManager, QWidget *parent = nullptr);

public slots:
    void refreshStatistics();

private:
    RecordManager *m_recordManager;

    // 汇总卡片
    QLabel *m_totalLabel;
    QLabel *m_avgConfidenceLabel;
    QLabel *m_topProvinceLabel;

    // 图表
    PieChartWidget *m_pieChart;
    BarChartWidget *m_barChart;

    // 日期筛选
    QDateEdit *m_dateFrom;
    QDateEdit *m_dateTo;
    QPushButton *m_refreshBtn;

    void setupUI();
    void updateSummaryLabels();
};

#endif
