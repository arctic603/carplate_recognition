#include "StatisticsWidget.h"
#include "data/RecordManager.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QtMath>
#include <QToolTip>

// ============================================================
//  PieChartWidget 实现 —— 使用 QPainter 手动绘制饼图
// ============================================================

PieChartWidget::PieChartWidget(QWidget *parent)
    : QWidget(parent)
{
    // 设置最小尺寸，确保图表有足够的绘制空间
    setMinimumSize(300, 300);
    initColors();
}

void PieChartWidget::setData(const QMap<QString, int> &data)
{
    m_data = data;
    // 数据更新后，调用 update() 触发 paintEvent 重绘
    update();
}

// 初始化调色板 —— 提供 15+ 种高区分度的颜色
void PieChartWidget::initColors()
{
    m_colors = {
        QColor("#2196F3"),  // 蓝色
        QColor("#F44336"),  // 红色
        QColor("#4CAF50"),  // 绿色
        QColor("#FF9800"),  // 橙色
        QColor("#9C27B0"),  // 紫色
        QColor("#00BCD4"),  // 青色
        QColor("#FFEB3B"),  // 黄色
        QColor("#795548"),  // 棕色
        QColor("#E91E63"),  // 粉色
        QColor("#3F51B5"),  // 靛蓝
        QColor("#009688"),  // 蓝绿
        QColor("#FF5722"),  // 深橙
        QColor("#607D8B"),  // 蓝灰
        QColor("#8BC34A"),  // 浅绿
        QColor("#CDDC39"),  // 黄绿
        QColor("#673AB7"),  // 深紫
        QColor("#03A9F4"),  // 浅蓝
        QColor("#FFC107"),  // 琥珀
    };
}

void PieChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    // 开启抗锯齿，让圆弧和文字边缘更平滑
    painter.setRenderHint(QPainter::Antialiasing);
    // 开启文字抗锯齿
    painter.setRenderHint(QPainter::TextAntialiasing);

    // --- 填充背景 ---
    painter.fillRect(rect(), Qt::white);

    // --- 绘制标题 ---
    // QPainter 的坐标系：原点在控件左上角，x 向右递增，y 向下递增
    QFont titleFont = painter.font();
    titleFont.setPixelSize(16);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor("#333333"));
    // 在顶部居中绘制标题文字
    painter.drawText(QRect(0, 5, width(), 30), Qt::AlignCenter, "省份分布统计");

    // --- 处理空数据情况 ---
    if (m_data.isEmpty()) {
        QFont emptyFont = painter.font();
        emptyFont.setPixelSize(14);
        painter.setFont(emptyFont);
        painter.setPen(QColor("#999999"));
        // 在控件正中央绘制提示文字
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    // --- 计算数据总和 ---
    int total = 0;
    for (int v : m_data) {
        total += v;
    }
    if (total == 0) {
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    // --- 确定饼图的绘制区域 ---
    // 饼图是一个正圆形，需要在矩形区域内绘制
    // 预留边距：顶部放标题，底部和两侧放标签
    int margin = 60;
    int titleHeight = 35;
    int availW = width() - 2 * margin;
    int availH = height() - titleHeight - margin - 30;  // 30 给底部标签留空间
    int side = qMin(availW, availH);
    // 饼图外接矩形的左上角坐标（相对于控件）
    int cx = (width() - side) / 2;
    int cy = titleHeight + (availH - side) / 2 + 10;
    QRect pieRect(cx, cy, side, side);

    // --- 绘制饼图扇形 ---
    // QPainter::drawPie 使用 1/16 度为单位的角度
    // 360度 = 5760（即 360 * 16）
    // 角度从 3 点钟方向（正右方）开始，逆时针为正方向
    // 我们从 12 点钟方向开始（即 90度 = 1440）
    int startAngle = 90 * 16;  // 起始角度：12 点钟方向

    // 收集每个扇形的中间角度，用于后续绘制标签引导线
    struct SliceInfo {
        QString label;
        int count;
        double percentage;
        double midAngleDeg;  // 扇形中间角度（度数，用于标签定位）
        QColor color;
    };
    QList<SliceInfo> slices;

    double currentAngleDeg = 90.0;  // 当前角度（度数），从 12 点钟开始

    int colorIndex = 0;
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it) {
        double percentage = (double)it.value() / total;
        // 该扇形占据的角度（度数）
        double spanDeg = percentage * 360.0;
        // 扇形中间的角度，用于放置标签
        double midDeg = currentAngleDeg + spanDeg / 2.0;

        SliceInfo info;
        info.label = it.key();
        info.count = it.value();
        info.percentage = percentage;
        info.midAngleDeg = midDeg;
        info.color = m_colors[colorIndex % m_colors.size()];
        slices.append(info);

        // drawPie 的角度参数：startAngle 和 spanAngle，单位为 1/16 度
        // 顺时针绘制时 spanAngle 为负值
        int spanAngle = -qRound(spanDeg * 16);
        painter.setPen(Qt::NoPen);
        painter.setBrush(info.color);
        painter.drawPie(pieRect, startAngle, spanAngle);

        // 更新下一个扇形的起始角度
        startAngle += spanAngle;
        currentAngleDeg += spanDeg;
        colorIndex++;
    }

    // --- 绘制标签引导线和文字 ---
    // 引导线从扇形边缘向外延伸，末端连接文字标签
    QFont labelFont = painter.font();
    labelFont.setPixelSize(11);
    painter.setFont(labelFont);

    // 饼图中心点坐标
    double centerX = pieRect.center().x();
    double centerY = pieRect.center().y();
    double radius = side / 2.0;

    for (const auto &slice : slices) {
        // 将角度转换为弧度（Qt 的三角函数使用弧度）
        // 注意：屏幕坐标系 y 轴向下，所以角度方向与数学坐标系相反
        double rad = qDegreesToRadians(slice.midAngleDeg);
        // 引导线起点：扇形边缘（半径 * 0.85 处，稍微内缩一点更好看）
        double lineStartX = centerX + radius * 0.85 * qCos(rad);
        double lineStartY = centerY - radius * 0.85 * qSin(rad);
        // 引导线终点：距离圆心 radius * 1.2 的位置
        double lineEndX = centerX + radius * 1.15 * qCos(rad);
        double lineEndY = centerY - radius * 1.15 * qSin(rad);

        // 绘制引导线
        painter.setPen(QPen(QColor("#666666"), 1));
        painter.drawLine(QPointF(lineStartX, lineStartY),
                         QPointF(lineEndX, lineEndY));

        // 标签文字：省份名 + 百分比
        QString labelText = QString("%1 %2%")
                .arg(slice.label)
                .arg(slice.percentage * 100, 0, 'f', 1);

        // 根据标签在圆心的左侧还是右侧，决定文字对齐方式
        // qCos(rad) > 0 表示在右侧，< 0 表示在左侧
        int textX = qRound(lineEndX);
        int textY = qRound(lineEndY) - 8;
        QRect textRect;
        if (qCos(rad) >= 0) {
            // 右侧标签：文字在引导线终点右边
            textRect = QRect(textX + 3, textY, 100, 16);
            painter.setPen(QColor("#333333"));
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, labelText);
        } else {
            // 左侧标签：文字在引导线终点左边
            textRect = QRect(textX - 103, textY, 100, 16);
            painter.setPen(QColor("#333333"));
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, labelText);
        }
    }
}


// ============================================================
//  BarChartWidget 实现 —— 使用 QPainter 手动绘制柱状图
// ============================================================

BarChartWidget::BarChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);
}

void BarChartWidget::setData(const QMap<QDate, int> &data)
{
    m_data = data;
    // 将 QMap<QDate, int> 转换为 QList<QPair<QString, int>>
    // 便于 paintEvent 中按索引绘制
    m_bars.clear();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        // 日期格式化为 MM-dd，显示在 X 轴下方
        QString label = it.key().toString("MM-dd");
        m_bars.append(qMakePair(label, it.value()));
    }
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // --- 填充背景 ---
    painter.fillRect(rect(), Qt::white);

    // --- 绘制标题 ---
    QFont titleFont = painter.font();
    titleFont.setPixelSize(16);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor("#333333"));
    painter.drawText(QRect(0, 5, width(), 30), Qt::AlignCenter, "每日识别量统计");

    // --- 处理空数据情况 ---
    if (m_bars.isEmpty()) {
        QFont emptyFont = painter.font();
        emptyFont.setPixelSize(14);
        painter.setFont(emptyFont);
        painter.setPen(QColor("#999999"));
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    // --- 定义绘图区域边距 ---
    // 左侧留出空间给 Y 轴数字标签
    // 底部留出空间给 X 轴日期标签
    int marginLeft = 55;
    int marginRight = 20;
    int marginTop = 45;
    int marginBottom = 50;

    // 实际绘图区域（柱状图绑制在这个矩形内）
    QRect plotArea(marginLeft, marginTop,
                   width() - marginLeft - marginRight,
                   height() - marginTop - marginBottom);

    // --- 计算 Y 轴刻度 ---
    // 找到数据最大值，用于确定 Y 轴范围
    int maxVal = 0;
    for (const auto &bar : m_bars) {
        if (bar.second > maxVal) maxVal = bar.second;
    }
    // 如果最大值为 0，设为 1 避免除零错误
    if (maxVal == 0) maxVal = 1;

    // 将最大值向上取整到一个"好看"的数字，便于刻度标注
    // 例如：maxVal=23 -> niceMax=30, maxVal=150 -> niceMax=200
    int niceMax = maxVal;
    int magnitude = 1;
    while (magnitude <= niceMax) magnitude *= 10;
    magnitude /= 10;
    if (magnitude < 1) magnitude = 1;
    niceMax = ((niceMax / magnitude) + 1) * magnitude;

    // Y 轴分为 5 个刻度
    int tickCount = 5;
    QFont axisFont = painter.font();
    axisFont.setPixelSize(10);
    painter.setFont(axisFont);

    // --- 绘制 Y 轴刻度线和网格线 ---
    painter.setPen(QPen(QColor("#CCCCCC"), 1, Qt::DashLine));
    for (int i = 0; i <= tickCount; i++) {
        int value = (niceMax * i) / tickCount;
        // 将数值映射到 Y 坐标（注意：屏幕 Y 轴向下，数值越大 Y 越小）
        int y = plotArea.bottom() - (int)((double)i / tickCount * plotArea.height());
        // 绘制水平虚线网格
        if (i > 0) {
            painter.drawLine(plotArea.left(), y, plotArea.right(), y);
        }
        // 在 Y 轴左侧标注刻度数值
        painter.setPen(QColor("#666666"));
        QRect labelRect(0, y - 8, marginLeft - 5, 16);
        painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value));
        // 切换回虚线画笔用于下一条网格线
        painter.setPen(QPen(QColor("#CCCCCC"), 1, Qt::DashLine));
    }

    // --- 绘制坐标轴线 ---
    painter.setPen(QPen(QColor("#333333"), 2));
    // Y 轴（左侧竖线）
    painter.drawLine(plotArea.left(), plotArea.top(),
                     plotArea.left(), plotArea.bottom());
    // X 轴（底部横线）
    painter.drawLine(plotArea.left(), plotArea.bottom(),
                     plotArea.right(), plotArea.bottom());

    // --- 绘制柱状条 ---
    int barCount = m_bars.size();
    // 每根柱子的宽度 = 绘图区域宽度 / 柱子数量 * 0.7（留 30% 间距）
    double barWidth = (double)plotArea.width() / barCount * 0.7;
    // 柱子之间的间距
    double gap = (double)plotArea.width() / barCount * 0.3;

    // 当柱子太多时（>15），X 轴标签只显示一部分，避免文字重叠
    int labelStep = 1;
    if (barCount > 15) {
        labelStep = (barCount + 14) / 15;  // 大约显示 15 个标签
    }

    for (int i = 0; i < barCount; i++) {
        // 计算当前柱子的 X 位置和高度
        double x = plotArea.left() + i * (barWidth + gap) + gap / 2;
        double barHeight = (double)m_bars[i].second / niceMax * plotArea.height();
        double y = plotArea.bottom() - barHeight;

        // --- 使用线性渐变填充柱子 ---
        // QLinearGradient 定义渐变的起止点和颜色
        // 这里从上到下渐变，顶部颜色浅、底部颜色深，营造立体感
        QLinearGradient gradient(x, y, x, plotArea.bottom());
        QColor barColor("#2196F3");
        gradient.setColorAt(0, barColor.lighter(140));  // 顶部：浅蓝
        gradient.setColorAt(1, barColor);                // 底部：标准蓝

        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        // drawRoundedRect 绘制圆角矩形，圆角半径 2px
        painter.drawRoundedRect(QRectF(x, y, barWidth, barHeight), 2, 2);

        // --- 绘制柱子顶部的数值（如果空间足够） ---
        if (barWidth > 18 && m_bars[i].second > 0) {
            QFont valFont = painter.font();
            valFont.setPixelSize(9);
            painter.setFont(valFont);
            painter.setPen(QColor("#333333"));
            QRect valRect(qRound(x), qRound(y) - 16, qRound(barWidth), 14);
            painter.drawText(valRect, Qt::AlignCenter,
                             QString::number(m_bars[i].second));
        }

        // --- 绘制 X 轴日期标签（按 labelStep 间隔显示） ---
        if (i % labelStep == 0) {
            painter.setFont(axisFont);
            painter.setPen(QColor("#666666"));
            // 标签区域在 X 轴下方
            QRect lblRect(qRound(x) - 5, plotArea.bottom() + 5,
                          qRound(barWidth) + 10, 20);
            // 旋转文字 45 度以节省水平空间
            // QPainter 的 save/rotate/restore 实现文字旋转：
            //   1. save() 保存当前坐标系状态
            //   2. translate() 移动原点到旋转中心
            //   3. rotate() 旋转坐标系
            //   4. drawText() 在新坐标系中绘制
            //   5. restore() 恢复原坐标系
            painter.save();
            painter.translate(lblRect.center().x(), lblRect.top());
            painter.rotate(-45);
            painter.drawText(0, 0, m_bars[i].first);
            painter.restore();
        }
    }
}


// ============================================================
//  StatisticsWidget 主控件实现
// ============================================================

StatisticsWidget::StatisticsWidget(RecordManager *recordManager, QWidget *parent)
    : QWidget(parent)
    , m_recordManager(recordManager)
{
    setupUI();
    // 初始化完成后立即加载一次统计数据
    refreshStatistics();
}

void StatisticsWidget::setupUI()
{
    // === 主布局：垂直排列 ===
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ============================
    //  顶部：3 个汇总卡片
    // ============================
    QHBoxLayout *cardsLayout = new QHBoxLayout;
    cardsLayout->setSpacing(12);

    // 卡片样式 —— QFrame 带圆角背景和阴影效果
    QString cardStyle = "QFrame { background-color: #f0f0f0; border-radius: 8px; padding: 10px; }";
    QString cardTitleStyle = "font-size: 12px; color: #666666;";

    // --- 卡片1：总识别数量 ---
    QFrame *card1 = new QFrame;
    card1->setStyleSheet(cardStyle);
    QVBoxLayout *card1Layout = new QVBoxLayout(card1);
    card1Layout->setSpacing(4);
    QLabel *card1Title = new QLabel("总识别数量");
    card1Title->setStyleSheet(cardTitleStyle);
    m_totalLabel = new QLabel("0");
    m_totalLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2196F3;");
    m_totalLabel->setAlignment(Qt::AlignCenter);
    card1Layout->addWidget(card1Title, 0, Qt::AlignCenter);
    card1Layout->addWidget(m_totalLabel);
    cardsLayout->addWidget(card1);

    // --- 卡片2：平均置信度 ---
    QFrame *card2 = new QFrame;
    card2->setStyleSheet(cardStyle);
    QVBoxLayout *card2Layout = new QVBoxLayout(card2);
    card2Layout->setSpacing(4);
    QLabel *card2Title = new QLabel("平均置信度");
    card2Title->setStyleSheet(cardTitleStyle);
    m_avgConfidenceLabel = new QLabel("0%");
    m_avgConfidenceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    m_avgConfidenceLabel->setAlignment(Qt::AlignCenter);
    card2Layout->addWidget(card2Title, 0, Qt::AlignCenter);
    card2Layout->addWidget(m_avgConfidenceLabel);
    cardsLayout->addWidget(card2);

    // --- 卡片3：最多省份 ---
    QFrame *card3 = new QFrame;
    card3->setStyleSheet(cardStyle);
    QVBoxLayout *card3Layout = new QVBoxLayout(card3);
    card3Layout->setSpacing(4);
    QLabel *card3Title = new QLabel("识别最多省份");
    card3Title->setStyleSheet(cardTitleStyle);
    m_topProvinceLabel = new QLabel("-");
    m_topProvinceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #FF9800;");
    m_topProvinceLabel->setAlignment(Qt::AlignCenter);
    card3Layout->addWidget(card3Title, 0, Qt::AlignCenter);
    card3Layout->addWidget(m_topProvinceLabel);
    cardsLayout->addWidget(card3);

    mainLayout->addLayout(cardsLayout);

    // ============================
    //  中部：图表区域（饼图 + 柱状图，使用 QSplitter 可拖拽分割）
    // ============================
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    m_pieChart = new PieChartWidget;
    m_barChart = new BarChartWidget;

    splitter->addWidget(m_pieChart);
    splitter->addWidget(m_barChart);
    // 设置初始比例：饼图占 2 份，柱状图占 3 份
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter, 1);  // stretch=1 让图表区域占据剩余空间

    // ============================
    //  底部：日期范围选择 + 刷新按钮
    // ============================
    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->setSpacing(8);

    filterLayout->addWidget(new QLabel("起始日期:"));
    m_dateFrom = new QDateEdit;
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("yyyy-MM-dd");
    // 默认日期范围：最近 30 天
    m_dateFrom->setDate(QDate::currentDate().addDays(-30));
    filterLayout->addWidget(m_dateFrom);

    filterLayout->addWidget(new QLabel("结束日期:"));
    m_dateTo = new QDateEdit;
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("yyyy-MM-dd");
    m_dateTo->setDate(QDate::currentDate());
    filterLayout->addWidget(m_dateTo);

    filterLayout->addStretch();  // 弹性空间把按钮推到右侧

    m_refreshBtn = new QPushButton("刷新统计");
    m_refreshBtn->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; "
        "border-radius: 4px; padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #1565C0; }"
    );
    filterLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(filterLayout);

    // --- 连接信号槽 ---
    // 点击刷新按钮时重新加载统计数据
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &StatisticsWidget::refreshStatistics);
}

void StatisticsWidget::updateSummaryLabels()
{
    // 更新总识别数量
    int total = m_recordManager->totalCount();
    m_totalLabel->setText(QString::number(total));

    // 更新平均置信度（转换为百分比显示）
    double avgConf = m_recordManager->averageConfidence();
    m_avgConfidenceLabel->setText(QString("%1%").arg(avgConf * 100, 0, 'f', 1));

    // 更新识别最多的省份
    QMap<QString, int> provinceCount = m_recordManager->countByProvince();
    if (!provinceCount.isEmpty()) {
        // 遍历找到数量最多的省份
        QString topProvince;
        int topCount = 0;
        for (auto it = provinceCount.constBegin(); it != provinceCount.constEnd(); ++it) {
            if (it.value() > topCount) {
                topCount = it.value();
                topProvince = it.key();
            }
        }
        m_topProvinceLabel->setText(QString("%1 (%2条)").arg(topProvince).arg(topCount));
    } else {
        m_topProvinceLabel->setText("-");
    }
}

void StatisticsWidget::refreshStatistics()
{
    // --- 更新饼图数据 ---
    // countByProvince() 返回 QMap<QString, int>：省份名 -> 识别次数
    QMap<QString, int> provinceData = m_recordManager->countByProvince();
    m_pieChart->setData(provinceData);

    // --- 更新柱状图数据 ---
    // countByDate() 返回 QMap<QDate, int>：日期 -> 当日识别次数
    QDate from = m_dateFrom->date();
    QDate to = m_dateTo->date();
    QMap<QDate, int> dateData = m_recordManager->countByDate(from, to);
    m_barChart->setData(dateData);

    // --- 更新汇总卡片 ---
    updateSummaryLabels();

    // --- 强制重绘图表（setData 内部已调用 update()，这里显式调用以确保刷新） ---
    m_pieChart->update();
    m_barChart->update();
}
