#include "RecognitionWidget.h"
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
#include <QFuture>
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
    QFuture<void> future = QtConcurrent::run([this, imgCopy]() {
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
