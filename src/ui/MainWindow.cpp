#include "MainWindow.h"
#include "RecognitionWidget.h"
#include "HistoryWidget.h"
#include "StatisticsWidget.h"
#include "CameraWidget.h"
#include "../core/PlateRecognizer.h"
#include "../data/RecordManager.h"

#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>

// -------------------------------------------------------
// 构造 / 析构
// -------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

MainWindow::~MainWindow() = default;

// -------------------------------------------------------
// 界面构建
// -------------------------------------------------------
void MainWindow::setupUI()
{
    // --- 窗口基本属性 ---
    setWindowTitle(QStringLiteral("车牌识别管理系统 v1.0"));
    setMinimumSize(1200, 800);

    // --- 创建核心业务对象 ---
    m_recognizer = new PlateRecognizer(this);

    // 记录文件存储在 AppDataLocation/records.json
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    QString recordsPath = dataPath + "/records.json";
    m_recordManager = new RecordManager(recordsPath, this);

    // --- 创建三个功能 Tab 页 ---
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    m_recognitionWidget = new RecognitionWidget(m_recognizer, m_recordManager, this);
    m_historyWidget     = new HistoryWidget(m_recordManager, this);
    m_statisticsWidget  = new StatisticsWidget(m_recordManager, this);

    m_tabWidget->addTab(m_recognitionWidget, QStringLiteral("车牌识别"));
    m_tabWidget->addTab(m_historyWidget,     QStringLiteral("识别记录"));
    m_tabWidget->addTab(m_statisticsWidget,  QStringLiteral("数据统计"));

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // 当识别页完成一次识别后，刷新历史和统计
    connect(m_recognitionWidget, &RecognitionWidget::recognitionCompleted,
            this, [this](const Record &) {
        m_historyWidget->refreshTable();
        m_statisticsWidget->refreshStatistics();
        statusBar()->showMessage(QStringLiteral("识别完成，记录已保存"), 5000);
    });

    // --- 菜单栏、状态栏 ---
    setupMenuBar();
    statusBar()->showMessage(QStringLiteral("就绪"));
}

// -------------------------------------------------------
// 菜单栏
// -------------------------------------------------------
void MainWindow::setupMenuBar()
{
    // ===== 文件菜单 =====
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));

    QAction *actOpen = fileMenu->addAction(QStringLiteral("打开图片(&O)"));
    actOpen->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpenImage);

    QAction *actExport = fileMenu->addAction(QStringLiteral("导出CSV(&E)"));
    actExport->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    connect(actExport, &QAction::triggered, this, &MainWindow::onExportCsv);

    fileMenu->addSeparator();

    QAction *actQuit = fileMenu->addAction(QStringLiteral("退出(&Q)"));
    actQuit->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
    connect(actQuit, &QAction::triggered, this, &QMainWindow::close);

    // ===== 工具菜单 =====
    QMenu *toolMenu = menuBar()->addMenu(QStringLiteral("工具(&T)"));

    QAction *actCamera = toolMenu->addAction(QStringLiteral("打开摄像头"));
    connect(actCamera, &QAction::triggered, this, &MainWindow::onOpenCamera);

    QAction *actClear = toolMenu->addAction(QStringLiteral("清空记录"));
    connect(actClear, &QAction::triggered, this, &MainWindow::onClearRecords);

    // ===== 帮助菜单 =====
    QMenu *helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));

    QAction *actAbout = helpMenu->addAction(QStringLiteral("关于"));
    connect(actAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

// -------------------------------------------------------
// 菜单动作槽函数
// -------------------------------------------------------
void MainWindow::onOpenImage()
{
    // 切换到识别页并打开图片选择
    m_tabWidget->setCurrentIndex(0);
    m_recognitionWidget->loadImageFromFile();
}

void MainWindow::onExportCsv()
{
    // 切换到历史页并执行导出
    m_tabWidget->setCurrentIndex(1);
    m_historyWidget->onExportCsv();
}

void MainWindow::onOpenCamera()
{
    // 弹出摄像头窗口，拍照后送至识别页
    CameraWidget *camera = new CameraWidget(this);
    camera->setAttribute(Qt::WA_DeleteOnClose);
    camera->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    connect(camera, &CameraWidget::imageCaptured,
            m_recognitionWidget, &RecognitionWidget::loadImageFromCamera);
    connect(camera, &CameraWidget::imageCaptured, this, [this]() {
        m_tabWidget->setCurrentIndex(0);
    });
    camera->show();
}

void MainWindow::onClearRecords()
{
    QMessageBox::StandardButton btn = QMessageBox::question(
        this,
        QStringLiteral("确认清空"),
        QStringLiteral("确定要清空所有识别记录吗？此操作不可撤销。"),
        QMessageBox::Yes | QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        m_recordManager->clearAll();
        m_historyWidget->refreshTable();
        m_statisticsWidget->refreshStatistics();
        statusBar()->showMessage(QStringLiteral("所有记录已清空"), 3000);
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(
        this,
        QStringLiteral("关于"),
        QStringLiteral(
            "<h2>车牌识别管理系统 v1.0</h2>"
            "<p>基于 <b>Qt6 + OpenCV</b> 开发的车牌识别管理系统。</p>"
            "<p><b>功能特点：</b></p>"
            "<ul>"
            "<li>支持本地图片识别与摄像头实时抓拍</li>"
            "<li>自动识别车牌号码，支持全国各省</li>"
            "<li>完整的识别记录查询与管理</li>"
            "<li>多维度数据统计与可视化图表</li>"
            "<li>支持 CSV 格式数据导出</li>"
            "</ul>"
            "<p>&copy; 2024 Arctic Team. All rights reserved.</p>"
        )
    );
}

// -------------------------------------------------------
// Tab 切换
// -------------------------------------------------------
void MainWindow::onTabChanged(int index)
{
    switch (index) {
    case 0: // 识别页 - 无需额外刷新
        break;
    case 1: // 记录页 - 刷新表格
        m_historyWidget->refreshTable();
        break;
    case 2: // 统计页 - 刷新图表
        m_statisticsWidget->refreshStatistics();
        break;
    default:
        break;
    }
}
