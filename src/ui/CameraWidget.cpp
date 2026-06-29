#include "CameraWidget.h"

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
    // Qt 6.11: QCamera no longer has setCamera(), recreate with device
    delete m_camera;
    m_camera = new QCamera(cameras.at(index), this);
    m_session->setCamera(m_camera);
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
        // Qt 6.11: QCamera no longer has setCamera(), recreate with device
    delete m_camera;
    m_camera = new QCamera(cameras.at(index), this);
    m_session->setCamera(m_camera);
    }
}
