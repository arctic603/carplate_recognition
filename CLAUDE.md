# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 项目概述

基于 **Qt 6.11.1 + OpenCV 4.12.0 + ONNX** 的车牌识别管理系统。  
采用两阶段识别流水线：YOLO 目标检测定位车牌 → ONNX OCR（CTC 解码）识别字符。

---

## 构建方式

**方式一：VS2022 + Qt VS Tools（推荐）**  
直接打开 `PlateRecognition.vcxproj`，Ctrl+B 编译，F5 运行。

**方式二：qmake + MSVC（须在 x64 Native Tools Command Prompt 中执行）**
```cmd
cd D:\Arctic_QT\PlateRecognition
qmake PlateRecognition.pro
nmake
```

**输出目录**：`bin/`  
Post-build 步骤会自动将 OpenCV DLL、ONNX 模型和 Qt DLL（通过 `windeployqt`）复制到 `bin/`。

### 依赖路径约定
- OpenCV：`D:/opencv4/opencv/build`（路径硬编码在 `.pro` 和 `.vcxproj` 中，更换环境须修改 `OPENCV_DIR`）
- ONNX 模型：`models/`（由 `car_plate_detect.onnx` 和 `plate_rec.onnx` 组成，须从 ModelScope 下载后放入此目录）

---

## 代码架构

### 核心识别层（`src/core/`）

三个类构成完整识别流水线：

```
输入图像 → PlateDetector → PlateRegion[] → PlateOCR → 识别结果
                    ↑                            ↑
              car_plate_detect.onnx        plate_rec.onnx
```

- **`PlateDetector`**：YOLO（640×640 letterbox 输入）检测车牌位置，若 YOLO 未加载则回退到 Sobel + HSV 形态学方法。输出 `PlateRegion`（含 boundingBox、BGR/灰度裁剪图、置信度）。
- **`PlateOCR`**：接收 BGR 裁剪图，缩放至 168×48，经归一化后送入 ONNX 网络，用 CTC 贪心解码输出字符串及置信度。字符集 78 个（含 CTC blank）。
- **`PlateRecognizer`**（门面）：持有上述两个对象，对外暴露 `recognize(cv::Mat)` 和 `recognizeFromFile(QString)` 两个接口，通过 Qt 信号报告进度和结果。`RecognitionResult` 用 `Q_DECLARE_METATYPE` 注册，可通过队列信号跨线程传递。

### UI 层（`src/ui/`）

- **`MainWindow`**：`QMainWindow` + `QTabWidget`，三个标签分别挂载 `RecognitionWidget`、`HistoryWidget`、`StatisticsWidget`；持有唯一的 `PlateRecognizer*` 和 `RecordManager*` 并注入给子部件。
- **`RecognitionWidget`**：支持文件/摄像头两种输入，调用识别后通过 `recognitionCompleted(Record)` 信号通知 `MainWindow` 持久化。
- **`HistoryWidget`**：展示历史记录，支持按车牌关键字、日期、省份筛选，以及删除、CSV 导出。
- **`StatisticsWidget`**：通过 SQL 聚合展示省份分布和按日计数趋势。
- **`CameraWidget`**：Qt Multimedia 实时取帧，通过信号将 `QImage` 送往 `RecognitionWidget`。

### 数据层（`src/data/`）

- **`Record`**：纯数据结构，含 `id`、`plateNumber`、`confidence`、`imagePath`、`timestamp`、`source`、`province`。
- **`RecordManager`**：`QSqlDatabase`（SQLite）CRUD，统计查询在 SQL 侧聚合（`countByProvince`、`countByDate`、`averageConfidence`）。DB 路径由 `MainWindow` 传入构造函数。

### 工具层（`src/utils/`）

- **`CsvExporter`**：`QList<Record>` → CSV 文件。
- **`ImageConverter`**：`cv::Mat` ↔ `QImage` 互转。

---

## 关键约定

- **文件读写**：本机受亿赛通透明加密管控，`.cpp`/`.h` 等源码文件**必须通过 Python 进程**读取，禁止使用 Shell `cat`/`echo` 或 Node.js 操作。
- **Qt 版本**：锁定 Qt **6.11.1**（MSVC 2022 64-bit），`PlateRecognition.pro` 中已声明 `c++17`。
- **ONNX 模型不在 Git 中存储**：须手动从 [ModelScope](https://modelscope.cn/models/supersong/onnxocr_model) 下载，放入 `models/`。
- **无独立测试套件**：`tools/` 目录下有 Python 工具脚本（代码生成、验证），不参与 qmake 编译。
- **信号跨线程**：若在工作线程调用 `PlateRecognizer`，需用 `Qt::QueuedConnection` 连接信号，或以 `QFuture` 包裹并在主线程回调。
