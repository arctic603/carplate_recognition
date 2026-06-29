#ifndef RECOGNITIONWIDGET_H
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
