#ifndef MAINWINDOW_H
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
