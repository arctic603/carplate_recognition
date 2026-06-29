#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成车牌识别项目 UI 层所有源文件
共 10 个文件: 5 个 .h + 5 个 .cpp
"""

import os

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UI_DIR = os.path.join(BASE_DIR, "src", "ui")

os.makedirs(UI_DIR, exist_ok=True)

FILES = {}

# ============================================================
# 1. MainWindow.h
# ============================================================
FILES["MainWindow.h"] = r'''#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTabWidget;
class QAction;

class PlateRecognizer;
class RecordManager;
class RecognitionWidget;
class HistoryWidget;
class StatisticsWidget;

/**
 * @brief 主窗口类
 *
 * 车牌识别管理系统的主窗口，包含三个功能标签页：
 * 车牌识别、识别记录、数据统计。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    /** 菜单动作：打开图片 */
    void onOpenImage();
    /** 菜单动作：导出 CSV */
    void onExportCsv();
    /** 菜单动作：打开摄像头 */
    void onOpenCamera();
    /** 菜单动作：清空所有记录 */
    void onClearRecords();
    /** 菜单动作：关于对话框 */
    void onAbout();
    /** Tab 页切换时刷新对应页数据 */
    void onTabChanged(int index);

private:
    /** 构建界面 */
    void setupUI();
    /** 构建菜单栏 */
    void setupMenuBar();

    // --- 核心业务对象 ---
    PlateRecognizer  *m_recognizer      = nullptr;
    RecordManager    *m_recordManager   = nullptr;

    // --- 界面组件 ---
    QTabWidget        *m_tabWidget       = nullptr;
    RecognitionWidget *m_recognitionWidget = nullptr;
    HistoryWidget     *m_historyWidget   = nullptr;
    StatisticsWidget  *m_statisticsWidget = nullptr;
};

#endif // MAINWINDOW_H
'''

# ============================================================
# 2. MainWindow.cpp
# ============================================================
FILES["MainWindow.cpp"] = r'''#include "MainWindow.h"
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
'''

# ============================================================
# 3. RecognitionWidget.h
# ============================================================
FILES["RecognitionWidget.h"] = r'''#ifndef RECOGNITIONWIDGET_H
#define RECOGNITIONWIDGET_H

#include <QWidget>
#include <QImage>

#include "../core/PlateRecognizer.h"
#include "../data/Record.h"

class QLabel;
class QPushButton;
class QProgressBar;
class QTextEdit;
class RecordManager;

namespace cv { class Mat; }

/**
 * @brief 车牌识别功能页面
 *
 * 提供图片加载（本地文件/摄像头）、异步车牌识别、
 * 结果展示及记录保存等功能。
 */
class RecognitionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecognitionWidget(PlateRecognizer *recognizer,
                               RecordManager   *recordManager,
                               QWidget *parent = nullptr);

public slots:
    /** 从本地文件加载图片 */
    void loadImageFromFile();
    /** 从摄像头拍照加载图片 */
    void loadImageFromCamera(const QImage &image);
    /** 开始异步识别 */
    void startRecognition();
    /** 保存当前识别结果到记录 */
    void saveResult();

signals:
    /** 识别完成并保存记录后发出 */
    void recognitionCompleted(const Record &record);

private slots:
    /** 识别完成回调（主线程） */
    void onRecognitionFinished(const RecognitionResult &result);

private:
    /** 构建界面 */
    void setupUI();
    /** 弹出摄像头窗口 */
    void openCamera();
    /** 根据车牌号推断省份 */
    static QString inferProvince(const QString &plate);

    // --- 业务对象 ---
    PlateRecognizer *m_recognizer     = nullptr;
    RecordManager   *m_recordManager  = nullptr;

    // --- 界面组件 ---
    QLabel       *m_imageLabel      = nullptr;  // 左侧大图预览
    QPushButton  *m_btnLoadFile     = nullptr;
    QPushButton  *m_btnOpenCamera   = nullptr;
    QPushButton  *m_btnRecognize    = nullptr;
    QProgressBar *m_progressBar     = nullptr;
    QLabel       *m_resultLabel     = nullptr;  // 车牌号（大字体）
    QLabel       *m_confidenceLabel = nullptr;  // 置信度
    QLabel       *m_platePreview    = nullptr;  // 裁剪车牌图
    QPushButton  *m_btnSave         = nullptr;
    QTextEdit    *m_logText         = nullptr;

    // --- 状态数据 ---
    cv::Mat             m_currentImage;   // 当前加载的图片
    RecognitionResult   m_lastResult;     // 最近一次识别结果
    QString             m_imageSource;    // 图片来源 ("本地图片"/"摄像头")
};

#endif // RECOGNITIONWIDGET_H
'''

# ============================================================
# 4. RecognitionWidget.cpp
# ============================================================
FILES["RecognitionWidget.cpp"] = r'''#include "RecognitionWidget.h"
#include "CameraWidget.h"
#include "../data/RecordManager.h"
#include "../utils/ImageConverter.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QFont>
#include <QMap>
#include <QDateTime>
#include <QtConcurrent>
#include <QFrame>

#include <opencv2/opencv.hpp>

// -------------------------------------------------------
// 构造
// -------------------------------------------------------
RecognitionWidget::RecognitionWidget(PlateRecognizer *recognizer,
                                     RecordManager   *recordManager,
                                     QWidget *parent)
    : QWidget(parent)
    , m_recognizer(recognizer)
    , m_recordManager(recordManager)
{
    setupUI();

    // 连接识别器的进度信号
    connect(m_recognizer, &PlateRecognizer::progressChanged,
            this, [this](int percent, const QString &stage) {
        m_progressBar->setValue(percent);
        m_logText->append(QStringLiteral("[%1%] %2").arg(percent).arg(stage));
    });
}

// -------------------------------------------------------
// 界面构建
// -------------------------------------------------------
void RecognitionWidget::setupUI()
{
    // ===== 左侧：图片显示 =====
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setMinimumSize(400, 400);
    m_imageLabel->setStyleSheet(
        QStringLiteral("QLabel { background-color: #2b2b2b; border: 1px solid #555; }"));
    m_imageLabel->setText(QStringLiteral("请加载图片"));

    // ===== 右侧：控制面板 =====
    QWidget *controlPanel = new QWidget(this);
    QVBoxLayout *ctrlLayout = new QVBoxLayout(controlPanel);
    ctrlLayout->setSpacing(8);

    // --- 按钮组 ---
    m_btnLoadFile = new QPushButton(QStringLiteral("选择本地图片"), this);
    m_btnOpenCamera = new QPushButton(QStringLiteral("打开摄像头"), this);
    m_btnRecognize = new QPushButton(QStringLiteral("开始识别"), this);

    m_btnLoadFile->setMinimumHeight(36);
    m_btnOpenCamera->setMinimumHeight(36);
    m_btnRecognize->setMinimumHeight(40);
    m_btnRecognize->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #1976D2; color: white; font-weight: bold; }"
                        "QPushButton:hover { background-color: #1565C0; }"));

    ctrlLayout->addWidget(m_btnLoadFile);
    ctrlLayout->addWidget(m_btnOpenCamera);
    ctrlLayout->addWidget(m_btnRecognize);

    // --- 分隔线 ---
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    ctrlLayout->addWidget(separator);

    // --- 进度条 ---
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setVisible(false);
    ctrlLayout->addWidget(m_progressBar);

    // --- 识别结果标签（大字体） ---
    m_resultLabel = new QLabel(QStringLiteral("等待识别"), this);
    QFont resultFont = m_resultLabel->font();
    resultFont.setPointSize(22);
    resultFont.setBold(true);
    m_resultLabel->setFont(resultFont);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    m_resultLabel->setStyleSheet(
        QStringLiteral("QLabel { color: #4CAF50; padding: 8px; }"));
    ctrlLayout->addWidget(m_resultLabel);

    // --- 置信度标签 ---
    m_confidenceLabel = new QLabel(this);
    m_confidenceLabel->setAlignment(Qt::AlignCenter);
    m_confidenceLabel->setStyleSheet(
        QStringLiteral("QLabel { color: #888; font-size: 13px; }"));
    ctrlLayout->addWidget(m_confidenceLabel);

    // --- 车牌裁剪预览 ---
    m_platePreview = new QLabel(this);
    m_platePreview->setAlignment(Qt::AlignCenter);
    m_platePreview->setFixedSize(200, 60);
    m_platePreview->setStyleSheet(
        QStringLiteral("QLabel { background-color: #333; border: 1px solid #666; }"));
    ctrlLayout->addWidget(m_platePreview, 0, Qt::AlignCenter);

    // --- 保存记录按钮 ---
    m_btnSave = new QPushButton(QStringLiteral("保存记录"), this);
    m_btnSave->setEnabled(false);
    m_btnSave->setMinimumHeight(36);
    ctrlLayout->addWidget(m_btnSave);

    // --- 日志文本框 ---
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setMaximumHeight(150);
    m_logText->setPlaceholderText(QStringLiteral("处理日志..."));
    ctrlLayout->addWidget(m_logText);

    ctrlLayout->addStretch();

    // ===== 使用 QSplitter 左右分栏 =====
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_imageLabel);
    splitter->addWidget(controlPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    // ===== 信号连接 =====
    connect(m_btnLoadFile,   &QPushButton::clicked, this, &RecognitionWidget::loadImageFromFile);
    connect(m_btnOpenCamera, &QPushButton::clicked, this, &RecognitionWidget::openCamera);
    connect(m_btnRecognize,  &QPushButton::clicked, this, &RecognitionWidget::startRecognition);
    connect(m_btnSave,       &QPushButton::clicked, this, &RecognitionWidget::saveResult);
}

// -------------------------------------------------------
// 加载本地图片
// -------------------------------------------------------
void RecognitionWidget::loadImageFromFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择图片"),
        QString(),
        QStringLiteral("图片文件 (*.jpg *.jpeg *.png *.bmp *.tiff);;所有文件 (*)"));
    if (filePath.isEmpty())
        return;

    // 使用 OpenCV 加载图片
    m_currentImage = cv::imread(filePath.toStdString());
    if (m_currentImage.empty()) {
        QMessageBox::warning(this, QStringLiteral("加载失败"),
                             QStringLiteral("无法读取图片文件，请检查路径和格式。"));
        return;
    }

    // 转换为 QImage 并显示
    QImage qimg = ImageConverter::matToQImage(m_currentImage);
    m_imageLabel->setPixmap(QPixmap::fromImage(qimg).scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    m_imageSource = QStringLiteral("本地图片");
    m_resultLabel->setText(QStringLiteral("等待识别"));
    m_confidenceLabel->clear();
    m_platePreview->clear();
    m_btnSave->setEnabled(false);
    m_logText->append(QStringLiteral("已加载图片: %1  (%2x%3)")
                          .arg(filePath)
                          .arg(m_currentImage.cols)
                          .arg(m_currentImage.rows));
}

// -------------------------------------------------------
// 从摄像头拍照加载
// -------------------------------------------------------
void RecognitionWidget::loadImageFromCamera(const QImage &image)
{
    if (image.isNull())
        return;

    m_currentImage = ImageConverter::qImageToMat(image);

    m_imageLabel->setPixmap(QPixmap::fromImage(image).scaled(
        m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    m_imageSource = QStringLiteral("摄像头");
    m_resultLabel->setText(QStringLiteral("等待识别"));
    m_confidenceLabel->clear();
    m_platePreview->clear();
    m_btnSave->setEnabled(false);
    m_logText->append(QStringLiteral("已从摄像头加载图片 (%1x%2)")
                          .arg(m_currentImage.cols)
                          .arg(m_currentImage.rows));
}

// -------------------------------------------------------
// 打开摄像头对话框
// -------------------------------------------------------
void RecognitionWidget::openCamera()
{
    CameraWidget *camera = new CameraWidget(this);
    camera->setAttribute(Qt::WA_DeleteOnClose);
    camera->setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    connect(camera, &CameraWidget::imageCaptured,
            this, &RecognitionWidget::loadImageFromCamera);
    camera->show();
}

// -------------------------------------------------------
// 异步识别（QtConcurrent + QMetaObject::invokeMethod）
// -------------------------------------------------------
void RecognitionWidget::startRecognition()
{
    if (m_currentImage.empty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先加载图片再进行识别。"));
        return;
    }

    m_logText->append(QStringLiteral("开始识别..."));
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_btnRecognize->setEnabled(false);
    m_resultLabel->setText(QStringLiteral("识别中..."));

    // 复制图片数据，避免线程竞争
    cv::Mat imgCopy = m_currentImage.clone();

    // 在工作线程执行识别，结果通过 QMetaObject::invokeMethod 回到主线程
    QtConcurrent::run([this, imgCopy]() {
        RecognitionResult result = m_recognizer->recognize(imgCopy);
        // 通过 QMetaObject::invokeMethod 将结果安全传回主线程
        QMetaObject::invokeMethod(this, [this, result]() {
            this->onRecognitionFinished(result);
        }, Qt::QueuedConnection);
    });
}

// -------------------------------------------------------
// 识别结果回调（主线程）
// -------------------------------------------------------
void RecognitionWidget::onRecognitionFinished(const RecognitionResult &result)
{
    m_progressBar->setVisible(false);
    m_btnRecognize->setEnabled(true);
    m_lastResult = result;

    if (!result.success) {
        m_resultLabel->setText(QStringLiteral("识别失败"));
        m_resultLabel->setStyleSheet(QStringLiteral("QLabel { color: #F44336; padding: 8px; }"));
        m_confidenceLabel->clear();
        m_logText->append(QStringLiteral("识别失败: %1").arg(result.errorMessage));
        return;
    }

    // 显示车牌号
    m_resultLabel->setText(result.plateNumber);
    m_resultLabel->setStyleSheet(QStringLiteral("QLabel { color: #4CAF50; padding: 8px; }"));

    // 显示置信度
    m_confidenceLabel->setText(QStringLiteral("置信度: %1%")
                                   .arg(result.confidence * 100, 0, 'f', 1));

    // 显示裁剪的车牌图
    if (!result.plateImage.empty()) {
        QImage plateQImg = ImageConverter::matToQImage(result.plateImage);
        m_platePreview->setPixmap(QPixmap::fromImage(plateQImg).scaled(
            m_platePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // 启用保存按钮
    m_btnSave->setEnabled(true);
    m_logText->append(QStringLiteral("识别成功: %1 (置信度: %2%)")
                          .arg(result.plateNumber)
                          .arg(result.confidence * 100, 0, 'f', 1));
}

// -------------------------------------------------------
// 保存识别记录
// -------------------------------------------------------
void RecognitionWidget::saveResult()
{
    if (!m_lastResult.success) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("没有可保存的识别结果。"));
        return;
    }

    // 准备存储目录
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString imagesDir = dataPath + "/images";
    QDir().mkpath(imagesDir);

    // 生成唯一 ID
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 保存原始图片
    QString imagePath = imagesDir + "/" + id + ".jpg";
    cv::imwrite(imagePath.toStdString(), m_currentImage);

    // 保存车牌裁剪图
    if (!m_lastResult.plateImage.empty()) {
        QString platePath = imagesDir + "/" + id + "_plate.jpg";
        cv::imwrite(platePath.toStdString(), m_lastResult.plateImage);
    }

    // 构建 Record
    Record record;
    record.id          = id;
    record.plateNumber = m_lastResult.plateNumber;
    record.imagePath   = imagePath;
    record.source      = m_imageSource;
    record.province    = inferProvince(m_lastResult.plateNumber);
    record.confidence  = m_lastResult.confidence;
    record.timestamp   = QDateTime::currentDateTime();

    m_recordManager->addRecord(record);

    m_btnSave->setEnabled(false);
    m_logText->append(QStringLiteral("记录已保存: %1").arg(record.plateNumber));

    emit recognitionCompleted(record);
}

// -------------------------------------------------------
// 根据车牌号首字符推断省份
// -------------------------------------------------------
QString RecognitionWidget::inferProvince(const QString &plate)
{
    if (plate.isEmpty())
        return QStringLiteral("未知");

    QChar first = plate.at(0);

    static const QMap<QChar, QString> provinceMap = {
        {QChar(u'京'), QStringLiteral("北京")},
        {QChar(u'津'), QStringLiteral("天津")},
        {QChar(u'沪'), QStringLiteral("上海")},
        {QChar(u'渝'), QStringLiteral("重庆")},
        {QChar(u'冀'), QStringLiteral("河北")},
        {QChar(u'豫'), QStringLiteral("河南")},
        {QChar(u'云'), QStringLiteral("云南")},
        {QChar(u'辽'), QStringLiteral("辽宁")},
        {QChar(u'黑'), QStringLiteral("黑龙江")},
        {QChar(u'湘'), QStringLiteral("湖南")},
        {QChar(u'皖'), QStringLiteral("安徽")},
        {QChar(u'鲁'), QStringLiteral("山东")},
        {QChar(u'新'), QStringLiteral("新疆")},
        {QChar(u'苏'), QStringLiteral("江苏")},
        {QChar(u'浙'), QStringLiteral("浙江")},
        {QChar(u'赣'), QStringLiteral("江西")},
        {QChar(u'鄂'), QStringLiteral("湖北")},
        {QChar(u'桂'), QStringLiteral("广西")},
        {QChar(u'甘'), QStringLiteral("甘肃")},
        {QChar(u'晋'), QStringLiteral("山西")},
        {QChar(u'蒙'), QStringLiteral("内蒙古")},
        {QChar(u'陕'), QStringLiteral("陕西")},
        {QChar(u'吉'), QStringLiteral("吉林")},
        {QChar(u'闽'), QStringLiteral("福建")},
        {QChar(u'贵'), QStringLiteral("贵州")},
        {QChar(u'粤'), QStringLiteral("广东")},
        {QChar(u'川'), QStringLiteral("四川")},
        {QChar(u'青'), QStringLiteral("青海")},
        {QChar(u'藏'), QStringLiteral("西藏")},
        {QChar(u'琼'), QStringLiteral("海南")},
        {QChar(u'宁'), QStringLiteral("宁夏")},
    };

    return provinceMap.value(first, QStringLiteral("未知"));
}
'''

# ============================================================
# 5. CameraWidget.h
# ============================================================
FILES["CameraWidget.h"] = r'''#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QImage>

class QMediaCaptureSession;
class QCamera;
class QVideoWidget;
class QImageCapture;
class QComboBox;
class QPushButton;

/**
 * @brief 摄像头预览与拍照窗口
 *
 * 使用 Qt6 Multimedia API 实现摄像头枚举、实时预览和拍照功能。
 * 拍照成功后通过 imageCaptured 信号传递 QImage。
 */
class CameraWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CameraWidget(QWidget *parent = nullptr);
    ~CameraWidget() override;

signals:
    /** 拍照成功，传出捕获的图像 */
    void imageCaptured(const QImage &image);

private slots:
    /** 开始摄像头预览 */
    void startPreview();
    /** 停止摄像头预览 */
    void stopPreview();
    /** 拍照 */
    void captureImage();
    /** 摄像头选择变更 */
    void onCameraChanged(int index);

private:
    /** 构建界面 */
    void setupUI();
    /** 枚举可用摄像头 */
    void enumerateCameras();

    // --- Qt6 Multimedia 对象 ---
    QMediaCaptureSession *m_session      = nullptr;
    QCamera              *m_camera       = nullptr;
    QVideoWidget         *m_videoWidget  = nullptr;
    QImageCapture        *m_imageCapture = nullptr;

    // --- 界面组件 ---
    QComboBox   *m_cameraSelector = nullptr;
    QPushButton *m_btnStart       = nullptr;
    QPushButton *m_btnStop        = nullptr;
    QPushButton *m_btnCapture     = nullptr;
};

#endif // CAMERAWIDGET_H
'''

# ============================================================
# 6. CameraWidget.cpp
# ============================================================
FILES["CameraWidget.cpp"] = r'''#include "CameraWidget.h"

#include <QMediaCaptureSession>
#include <QCamera>
#include <QVideoWidget>
#include <QImageCapture>
#include <QMediaDevices>
#include <QCameraDevice>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

// -------------------------------------------------------
// 构造 / 析构
// -------------------------------------------------------
CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("摄像头"));
    setMinimumSize(640, 520);
    setupUI();
    enumerateCameras();
}

CameraWidget::~CameraWidget()
{
    // 析构时确保停止摄像头
    if (m_camera && m_camera->isActive())
        m_camera->stop();
}

// -------------------------------------------------------
// 界面构建
// -------------------------------------------------------
void CameraWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- 摄像头选择行 ---
    QHBoxLayout *selectorLayout = new QHBoxLayout();
    selectorLayout->addWidget(new QLabel(QStringLiteral("摄像头:")));
    m_cameraSelector = new QComboBox(this);
    m_cameraSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    selectorLayout->addWidget(m_cameraSelector);
    mainLayout->addLayout(selectorLayout);

    // --- 视频预览区域 ---
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumSize(480, 360);
    m_videoWidget->setStyleSheet(
        QStringLiteral("background-color: #1a1a1a;"));
    mainLayout->addWidget(m_videoWidget, 1);

    // --- 控制按钮 ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnStart   = new QPushButton(QStringLiteral("开始预览"), this);
    m_btnStop    = new QPushButton(QStringLiteral("停止预览"), this);
    m_btnCapture = new QPushButton(QStringLiteral("拍照"), this);

    m_btnCapture->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #E53935; color: white; font-weight: bold; }"
                        "QPushButton:hover { background-color: #C62828; }"));

    btnLayout->addWidget(m_btnStart);
    btnLayout->addWidget(m_btnStop);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnCapture);
    mainLayout->addLayout(btnLayout);

    // --- 创建多媒体会话 ---
    m_session      = new QMediaCaptureSession(this);
    m_camera       = new QCamera(this);
    m_imageCapture = new QImageCapture(this);

    m_session->setCamera(m_camera);
    m_session->setVideoOutput(m_videoWidget);
    m_session->setImageCapture(m_imageCapture);

    // --- 信号连接 ---
    connect(m_btnStart,   &QPushButton::clicked, this, &CameraWidget::startPreview);
    connect(m_btnStop,    &QPushButton::clicked, this, &CameraWidget::stopPreview);
    connect(m_btnCapture, &QPushButton::clicked, this, &CameraWidget::captureImage);
    connect(m_cameraSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CameraWidget::onCameraChanged);

    // 拍照完成后获取图像
    connect(m_imageCapture, &QImageCapture::imageCaptured,
            this, [this](int id, const QImage &preview) {
        Q_UNUSED(id)
        emit imageCaptured(preview);
    });
}

// -------------------------------------------------------
// 枚举可用摄像头
// -------------------------------------------------------
void CameraWidget::enumerateCameras()
{
    m_cameraSelector->blockSignals(true);
    m_cameraSelector->clear();

    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &device : cameras) {
        m_cameraSelector->addItem(device.description(), device.id());
    }

    if (cameras.isEmpty()) {
        m_cameraSelector->addItem(QStringLiteral("未检测到摄像头"));
        m_btnStart->setEnabled(false);
        m_btnCapture->setEnabled(false);
    }

    m_cameraSelector->blockSignals(false);
}

// -------------------------------------------------------
// 开始预览
// -------------------------------------------------------
void CameraWidget::startPreview()
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    int index = m_cameraSelector->currentIndex();
    if (index < 0 || index >= cameras.size()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("没有可用的摄像头设备。"));
        return;
    }
    m_camera->setCamera(cameras.at(index));
    m_camera->start();
}

// -------------------------------------------------------
// 停止预览
// -------------------------------------------------------
void CameraWidget::stopPreview()
{
    if (m_camera && m_camera->isActive())
        m_camera->stop();
}

// -------------------------------------------------------
// 拍照
// -------------------------------------------------------
void CameraWidget::captureImage()
{
    if (!m_camera || !m_camera->isActive()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先启动摄像头预览。"));
        return;
    }
    m_imageCapture->capture();
}

// -------------------------------------------------------
// 摄像头切换
// -------------------------------------------------------
void CameraWidget::onCameraChanged(int index)
{
    stopPreview();
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (index >= 0 && index < cameras.size()) {
        m_camera->setCamera(cameras.at(index));
    }
}
'''

# ============================================================
# 7. HistoryWidget.h
# ============================================================
FILES["HistoryWidget.h"] = r'''#ifndef HISTORYWIDGET_H
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
'''

# ============================================================
# 8. HistoryWidget.cpp
# ============================================================
FILES["HistoryWidget.cpp"] = r'''#include "HistoryWidget.h"
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
'''

# ============================================================
# 9. StatisticsWidget.h
# ============================================================
FILES["StatisticsWidget.h"] = r'''#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>

class QLabel;
class QDateEdit;
class QPushButton;

namespace QtCharts {
    class QChartView;
}

class RecordManager;

/**
 * @brief 数据统计页面
 *
 * 展示识别总量、平均置信度、省份分布饼图、每日识别量柱状图等统计信息。
 */
class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsWidget(RecordManager *recordManager, QWidget *parent = nullptr);

public slots:
    /** 刷新所有统计数据和图表 */
    void refreshStatistics();

private slots:
    /** 日期范围变化时自动刷新 */
    void onDateRangeChanged();

private:
    /** 构建界面 */
    void setupUI();
    /** 更新顶部汇总卡片 */
    void updateSummaryCards();
    /** 构建省份分布饼图 */
    void buildPieChart();
    /** 构建每日识别量柱状图 */
    void buildBarChart();

    RecordManager *m_recordManager = nullptr;

    // --- 汇总卡片 ---
    QLabel *m_totalLabel        = nullptr;
    QLabel *m_avgConfLabel      = nullptr;
    QLabel *m_topProvinceLabel  = nullptr;

    // --- 图表 ---
    QtCharts::QChartView *m_provincePieChart = nullptr;
    QtCharts::QChartView *m_dailyBarChart    = nullptr;

    // --- 日期范围控制 ---
    QDateEdit   *m_dateFrom   = nullptr;
    QDateEdit   *m_dateTo     = nullptr;
    QPushButton *m_btnRefresh = nullptr;
};

#endif // STATISTICSWIDGET_H
'''

# ============================================================
# 10. StatisticsWidget.cpp
# ============================================================
FILES["StatisticsWidget.cpp"] = r'''#include "StatisticsWidget.h"
#include "../data/RecordManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QDateEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QDate>
#include <QFont>
#include <QColor>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

// -------------------------------------------------------
// 构造
// -------------------------------------------------------
StatisticsWidget::StatisticsWidget(RecordManager *recordManager, QWidget *parent)
    : QWidget(parent)
    , m_recordManager(recordManager)
{
    setupUI();
}

// -------------------------------------------------------
// 界面构建
// -------------------------------------------------------
void StatisticsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ===== 顶部汇总卡片 =====
    QHBoxLayout *cardsLayout = new QHBoxLayout();

    auto makeCard = [](const QString &title, QLabel *&valueLabel) -> QWidget * {
        QGroupBox *box = new QGroupBox(title);
        box->setAlignment(Qt::AlignCenter);
        QVBoxLayout *l = new QVBoxLayout(box);
        valueLabel = new QLabel(QStringLiteral("--"));
        QFont f = valueLabel->font();
        f.setPointSize(24);
        f.setBold(true);
        valueLabel->setFont(f);
        valueLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(valueLabel);
        box->setMinimumHeight(100);
        return box;
    };

    cardsLayout->addWidget(makeCard(QStringLiteral("总识别数"), m_totalLabel));
    cardsLayout->addWidget(makeCard(QStringLiteral("平均置信度"), m_avgConfLabel));
    cardsLayout->addWidget(makeCard(QStringLiteral("最多省份"), m_topProvinceLabel));

    mainLayout->addLayout(cardsLayout);

    // ===== 中部图表区域 =====
    QSplitter *chartSplitter = new QSplitter(Qt::Horizontal, this);

    m_provincePieChart = new QChartView(this);
    m_provincePieChart->setRenderHint(QPainter::Antialiasing);
    m_provincePieChart->setMinimumWidth(350);

    m_dailyBarChart = new QChartView(this);
    m_dailyBarChart->setRenderHint(QPainter::Antialiasing);
    m_dailyBarChart->setMinimumWidth(350);

    chartSplitter->addWidget(m_provincePieChart);
    chartSplitter->addWidget(m_dailyBarChart);
    chartSplitter->setStretchFactor(0, 1);
    chartSplitter->setStretchFactor(1, 1);
    mainLayout->addWidget(chartSplitter, 1);

    // ===== 底部日期范围控制 =====
    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel(QStringLiteral("日期范围:")));

    m_dateFrom = new QDateEdit(QDate::currentDate().addDays(-30), this);
    m_dateFrom->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateFrom->setCalendarPopup(true);

    m_dateTo = new QDateEdit(QDate::currentDate(), this);
    m_dateTo->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dateTo->setCalendarPopup(true);

    m_btnRefresh = new QPushButton(QStringLiteral("刷新统计"), this);
    m_btnRefresh->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #1976D2; color: white; "
                        "padding: 6px 20px; font-weight: bold; }"));

    dateLayout->addWidget(m_dateFrom);
    dateLayout->addWidget(new QLabel(QStringLiteral("至")));
    dateLayout->addWidget(m_dateTo);
    dateLayout->addStretch();
    dateLayout->addWidget(m_btnRefresh);
    mainLayout->addLayout(dateLayout);

    // ===== 信号连接 =====
    connect(m_dateFrom, &QDateEdit::dateChanged, this, &StatisticsWidget::onDateRangeChanged);
    connect(m_dateTo,   &QDateEdit::dateChanged, this, &StatisticsWidget::onDateRangeChanged);
    connect(m_btnRefresh, &QPushButton::clicked, this, &StatisticsWidget::refreshStatistics);
}

// -------------------------------------------------------
// 刷新所有统计数据
// -------------------------------------------------------
void StatisticsWidget::refreshStatistics()
{
    updateSummaryCards();
    buildPieChart();
    buildBarChart();
}

// -------------------------------------------------------
// 更新汇总卡片
// -------------------------------------------------------
void StatisticsWidget::updateSummaryCards()
{
    int total = m_recordManager->totalCount();
    m_totalLabel->setText(QString::number(total));

    double avgConf = m_recordManager->averageConfidence();
    m_avgConfLabel->setText(QStringLiteral("%1%").arg(avgConf * 100, 0, 'f', 1));

    // 找出识别最多的省份
    QMap<QString, int> provCounts = m_recordManager->countByProvince();
    QString topProvince = QStringLiteral("--");
    int maxCount = 0;
    for (auto it = provCounts.constBegin(); it != provCounts.constEnd(); ++it) {
        if (it.value() > maxCount) {
            maxCount = it.value();
            topProvince = it.key();
        }
    }
    if (maxCount > 0)
        m_topProvinceLabel->setText(QStringLiteral("%1 (%2)").arg(topProvince).arg(maxCount));
    else
        m_topProvinceLabel->setText(QStringLiteral("--"));
}

// -------------------------------------------------------
// 构建省份分布饼图
// -------------------------------------------------------
void StatisticsWidget::buildPieChart()
{
    QMap<QString, int> provCounts = m_recordManager->countByProvince();

    QPieSeries *series = new QPieSeries();

    // 配色列表
    QList<QColor> colors = {
        QColor(0x19, 0x76, 0xD2), QColor(0x4C, 0xAF, 0x50),
        QColor(0xFF, 0x98, 0x00), QColor(0xE5, 0x39, 0x35),
        QColor(0x9C, 0x27, 0xB0), QColor(0x00, 0xBC, 0xD4),
        QColor(0x79, 0x55, 0x48), QColor(0x60, 0x7D, 0x8B),
        QColor(0xFF, 0x57, 0x22), QColor(0x3F, 0x51, 0xB5),
    };
    int colorIdx = 0;

    for (auto it = provCounts.constBegin(); it != provCounts.constEnd(); ++it) {
        QPieSlice *slice = series->append(it.key(), it.value());
        slice->setLabelVisible(true);
        slice->setLabel(QStringLiteral("%1: %2").arg(it.key()).arg(it.value()));
        slice->setColor(colors.at(colorIdx % colors.size()));
        ++colorIdx;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QStringLiteral("各省份识别分布"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);

    // 删除旧图表
    if (m_provincePieChart->chart())
        delete m_provincePieChart->chart();
    m_provincePieChart->setChart(chart);
}

// -------------------------------------------------------
// 构建每日识别量柱状图
// -------------------------------------------------------
void StatisticsWidget::buildBarChart()
{
    QDate from = m_dateFrom->date();
    QDate to   = m_dateTo->date();
    QMap<QDate, int> dailyCounts = m_recordManager->countByDate(from, to);

    QBarSet *barSet = new QBarSet(QStringLiteral("识别数量"));
    barSet->setColor(QColor(0x19, 0x76, 0xD2));
    QStringList categories;

    QDate current = from;
    while (current <= to) {
        categories << current.toString(QStringLiteral("MM-dd"));
        *barSet << dailyCounts.value(current, 0);
        current = current.addDays(1);
    }

    QBarSeries *series = new QBarSeries();
    series->append(barSet);

    // X 轴：日期分类
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText(QStringLiteral("日期"));

    // Y 轴：识别数量
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(QStringLiteral("识别数量"));
    axisY->setLabelFormat(QStringLiteral("%d"));

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    chart->setTitle(QStringLiteral("每日识别量统计"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // 删除旧图表
    if (m_dailyBarChart->chart())
        delete m_dailyBarChart->chart();
    m_dailyBarChart->setChart(chart);
}

// -------------------------------------------------------
// 日期范围变化
// -------------------------------------------------------
void StatisticsWidget::onDateRangeChanged()
{
    refreshStatistics();
}
'''


# ============================================================
# 写入所有文件
# ============================================================
print(f"目标目录: {UI_DIR}")
print("-" * 60)

for filename, content in FILES.items():
    filepath = os.path.join(UI_DIR, filename)
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content.lstrip('\n'))
    print(f"[OK] {filename}")

print("-" * 60)
print(f"共生成 {len(FILES)} 个文件。")
