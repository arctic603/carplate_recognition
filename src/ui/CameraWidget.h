#ifndef CAMERAWIDGET_H
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
