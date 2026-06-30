# ============================================================
# 车牌识别管理系统 - qmake 项目文件
# Qt 6.11.1 (MSVC 2022 64-bit) + OpenCV 4.12.0
# ============================================================

QT += widgets multimedia multimediawidgets concurrent sql
CONFIG += c++17

TARGET = PlateRecognition
TEMPLATE = app

# ============================================================
# OpenCV 配置 (根据你的安装路径修改)
# ============================================================

OPENCV_DIR = D:/opencv4/opencv/build

INCLUDEPATH += $$OPENCV_DIR/include

# 区分 Debug / Release 链接不同的 lib
CONFIG(debug, debug|release) {
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib -lopencv_world4120d
} else {
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib -lopencv_world4120
}

# ============================================================
# 源文件
# ============================================================

SOURCES += \
    src/main.cpp \
    src/core/PlateDetector.cpp \
    src/core/PlateOCR.cpp \
    src/core/PlateRecognizer.cpp \
    src/data/RecordManager.cpp \
    src/ui/CameraWidget.cpp \
    src/ui/HistoryWidget.cpp \
    src/ui/MainWindow.cpp \
    src/ui/RecognitionWidget.cpp \
    src/ui/StatisticsWidget.cpp \
    src/utils/CsvExporter.cpp \
    src/utils/ImageConverter.cpp

HEADERS += \
    src/core/PlateDetector.h \
    src/core/PlateOCR.h \
    src/core/PlateRecognizer.h \
    src/data/Record.h \
    src/data/RecordManager.h \
    src/ui/CameraWidget.h \
    src/ui/HistoryWidget.h \
    src/ui/MainWindow.h \
    src/ui/RecognitionWidget.h \
    src/ui/StatisticsWidget.h \
    src/utils/CsvExporter.h \
    src/utils/ImageConverter.h

RESOURCES += \
    resources/resources.qrc

# ============================================================
# 头文件搜索路径
# ============================================================

INCLUDEPATH += src

# ============================================================
# 输出目录
# ============================================================

DESTDIR = $$OUT_PWD/bin
OBJECTS_DIR = $$OUT_PWD/obj
MOC_DIR = $$OUT_PWD/moc
RCC_DIR = $$OUT_PWD/rcc

# ============================================================
# Windows 配置: 自动部署 DLL
# ============================================================

win32 {
    # Release 模式关闭控制台
    CONFIG(release, debug|release): CONFIG += windows

    # 拷贝 OpenCV DLL
    CONFIG(debug, debug|release) {
        OPENCV_DLL = $$OPENCV_DIR/x64/vc16/bin/opencv_world4120d.dll
    } else {
        OPENCV_DLL = $$OPENCV_DIR/x64/vc16/bin/opencv_world4120.dll
    }

    QMAKE_POST_LINK += $$quote(copy /Y "$$shell_path($$OPENCV_DLL)" "$$shell_path($$DESTDIR)")

    # 拷贝 ONNX 模型文件到 bin/models/
    MODEL_SRC = $$PWD/models
    MODEL_DST = $$DESTDIR/models
    QMAKE_POST_LINK += $$escape_expand(\\n\\t) if not exist "$$shell_path($$MODEL_DST)" mkdir "$$shell_path($$MODEL_DST)"
    QMAKE_POST_LINK += $$escape_expand(\\n\\t) copy /Y "$$shell_path($$MODEL_SRC)\\plate_rec.onnx" "$$shell_path($$MODEL_DST)\\plate_rec.onnx"

    # 使用 windeployqt 自动部署 Qt DLL
    WINDEPLOYQT = $$[QT_HOST_BINS]/windeployqt.exe
    QMAKE_POST_LINK += $$escape_expand(\\n\\t) "$$WINDEPLOYQT" --no-translations --no-system-d3d-compiler --no-opengl-sw "$$DESTDIR/$$TARGET.exe"
}
