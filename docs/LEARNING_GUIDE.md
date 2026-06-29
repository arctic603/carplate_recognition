# 车牌识别管理系统 —— 小白学习指南

本文档是一份从零开始的学习路线，帮助你理解这个项目的每一部分。即使你之前没接触过 Qt 或 OpenCV，按照本指南的顺序阅读，也能逐步掌握整个项目。

---

## 学习路线总览

建议按以下顺序学习，每个阶段对应项目的不同文件：

```
第1章  C++ 基础回顾          （不需要看代码，快速过一遍即可）
第2章  Qt6 入门              → 重点看 main.cpp 和 MainWindow
第3章  理解项目结构           → 看 CMakeLists.txt 和各目录
第4章  数据层                → Record.h、RecordManager
第5章  工具类                → ImageConverter、CsvExporter
第6章  OpenCV 基础           → 了解 Mat 和基本操作
第7章  图像处理流水线         → PlateDetector（核心算法）
第8章  字符分割与识别         → CharacterClassifier
第9章  界面开发               → 各个 Widget 的实现
第10章 进阶方向              → 深度学习替代方案
```

---

## 第 1 章：C++ 基础回顾

在开始之前，你需要了解以下 C++ 概念（不需要精通，知道是什么意思就行）：

**类与对象**：C++ 用 `class` 定义类，类里有成员变量（数据）和成员函数（方法）。比如 `PlateDetector` 是一个类，`detect()` 是它的方法。

**指针与引用**：`PlateRecognizer *m_recognizer` 表示一个指向 PlateRecognizer 对象的指针。`&` 是引用，相当于给变量起了个别名。

**头文件与源文件**：`.h` 文件声明"有什么"（类定义、函数签名），`.cpp` 文件实现"怎么做"（具体代码）。`#include` 把声明引入到需要的地方。

**命名空间**：`cv::Mat` 表示 OpenCV 命名空间下的 Mat 类，`std::vector` 表示标准库命名空间下的 vector。防止不同库的同名类冲突。

**信号与槽（Qt 特有）**：Qt 的核心机制。一个对象发出信号（signal），另一个对象的槽函数（slot）接收并响应。比如按钮被点击（信号）→ 执行某个函数（槽）。

---

## 第 2 章：Qt6 入门

### 2.1 程序入口：main.cpp

打开 `src/main.cpp`，这是整个程序的起点：

```cpp
#include "ui/MainWindow.h"     // 引入主窗口类
#include <QApplication>        // Qt 应用程序类

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // 创建应用实例（必须有且仅有一个）
    app.setStyle("Fusion");        // 设置界面风格

    MainWindow window;             // 创建主窗口对象
    window.show();                 // 显示窗口（默认隐藏）

    return app.exec();             // 进入事件循环（程序在这里一直运行）
}
```

关键理解：`app.exec()` 启动事件循环后，程序就一直运行，等待用户操作（点击按钮、输入文字等），直到用户关闭窗口。

### 2.2 Qt 的信号与槽

这是 Qt 最核心的概念，用一个例子理解：

```cpp
// 当按钮被点击时，执行 onButtonClicked 函数
connect(myButton, &QPushButton::clicked, this, &MyClass::onButtonClicked);
```

- `myButton`：发出信号的对象
- `&QPushButton::clicked`：什么信号（按钮被点击了）
- `this`：谁来响应
- `&MyClass::onButtonClicked`：用什么函数响应

在本项目中你会看到大量 `connect` 调用，它们把界面操作和处理逻辑连接起来。

### 2.3 界面布局

Qt 用 **布局管理器** 自动排列控件：

| 布局类 | 排列方式 |
|--------|---------|
| QHBoxLayout | 水平排列（从左到右） |
| QVBoxLayout | 垂直排列（从上到下） |
| QSplitter | 可拖拽分割面板 |
| QTabWidget | 标签页切换 |

打开 `src/ui/MainWindow.cpp` 的 `setupUI()` 方法，你能看到如何用 `QTabWidget` 创建三个标签页。

### 2.4 练习建议

1. 打开 Qt Creator，创建一个最简单的 Qt Widgets Application
2. 在界面上拖一个按钮和一个标签
3. 写 `connect` 让按钮点击时改变标签文字
4. 运行看看效果

---

## 第 3 章：理解项目结构

### 3.1 CMakeLists.txt

打开项目根目录的 `CMakeLists.txt`，这是 CMake 的构建配置文件。逐段理解：

```cmake
cmake_minimum_required(VERSION 3.19)        # 最低 CMake 版本
project(PlateRecognition VERSION 1.0.0)     # 项目名和版本

set(CMAKE_CXX_STANDARD 17)                  # 使用 C++17 标准
set(CMAKE_AUTOMOC ON)                       # 自动处理 Qt 的 MOC（元对象编译器）
set(CMAKE_AUTORCC ON)                       # 自动处理资源文件
```

**AUTOMOC 是什么？** Qt 的类如果用了 `Q_OBJECT` 宏（为了支持信号与槽），就需要 MOC 工具预处理。`CMAKE_AUTOMOC ON` 让 CMake 自动完成这一步，你不用手动调用。

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets Charts Concurrent)
find_package(OpenCV REQUIRED)
```

`find_package` 告诉 CMake 去哪里找 Qt6 和 OpenCV 的头文件和库文件。如果找不到，CMake 会报错——这时候需要检查安装路径是否正确。

### 3.2 三层架构

本项目按职责分成三层：

**UI 层**（`src/ui/`）：负责界面显示和用户交互。不处理任何图像算法，只负责"展示结果"和"收集输入"。

**核心层**（`src/core/`）：负责车牌检测和字符识别。不关心界面长什么样，只接收图像、返回结果。

**数据层**（`src/data/` + `src/utils/`）：负责数据持久化（存到 JSON 文件）、格式转换、导出。

这种分层的好处是：修改界面不影响算法，修改算法不影响界面。各层之间通过清晰的接口（函数参数和返回值）通信。

---

## 第 4 章：数据层

### 4.1 数据模型：Record.h

打开 `src/data/Record.h`，这是最基础的数据结构：

```cpp
struct Record {
    QString id;             // 唯一标识符（UUID）
    QString plateNumber;    // 车牌号，如 "京A12345"
    double  confidence;     // 识别置信度，0.0 ~ 1.0
    QString imagePath;      // 原始图片保存路径
    QDateTime timestamp;    // 识别时间
    QString source;         // 来源："camera" 或 "file"
    QString province;       // 省份简称，如 "京"
};
```

**为什么需要这个结构体？** 每次识别成功后，需要把结果保存下来。这个结构体定义了"一条识别记录包含哪些信息"。

`toJson()` 和 `fromJson()` 方法负责把 Record 转换成 JSON 格式（方便存入文件）和从 JSON 恢复。

### 4.2 记录管理器：RecordManager

打开 `src/data/RecordManager.h` 和 `.cpp`。这个类管理所有识别记录：

**核心功能：**
- `addRecord()`：添加一条新记录
- `deleteRecord()`：删除指定记录
- `getAllRecords()`：获取所有记录
- `searchByPlate()`：按车牌号搜索
- `filterByDateRange()`：按日期范围筛选
- `countByProvince()`：统计各省份数量

**数据存储方式：** 所有记录保存在一个 JSON 文件中，格式如下：

```json
{
    "version": "1.0",
    "records": [
        {
            "id": "550e8400-...",
            "plateNumber": "京A12345",
            "confidence": 0.87,
            "timestamp": "2025-01-15T14:30:00"
        }
    ]
}
```

程序启动时从文件加载记录，每次增删记录后自动保存回文件。`QSaveFile` 保证写入的原子性——如果写入过程中程序崩溃，不会损坏原有文件。

---

## 第 5 章：工具类

### 5.1 图像格式转换：ImageConverter

Qt 用 `QImage` 表示图像，OpenCV 用 `cv::Mat`。两者内部数据格式不同：

| | QImage | cv::Mat |
|---|--------|---------|
| 颜色顺序 | RGB | BGR |
| 内存管理 | Qt 管理 | 自己管理（引用计数） |

`ImageConverter` 负责在两者之间转换。注意 `.copy()` 和 `.clone()` 的使用——它们执行深拷贝，防止一块内存被两个对象同时引用导致的崩溃。

### 5.2 CSV 导出：CsvExporter

CSV（逗号分隔值）是一种简单的表格数据格式，Excel 可以直接打开。`CsvExporter` 把识别记录导出为 CSV 文件。

关键点：文件开头写入 UTF-8 BOM（`\xEF\xBB\xBF`），这是让 Excel 正确识别中文的必要标记。没有 BOM 的话，Excel 打开中文 CSV 会显示乱码。

---

## 第 6 章：OpenCV 基础

### 6.1 cv::Mat —— OpenCV 的核心

`cv::Mat` 是 OpenCV 中表示图像（矩阵）的类。可以理解为一个二维数组，每个元素是一个像素。

```cpp
cv::Mat image = cv::imread("test.jpg");   // 读取图片
int rows = image.rows;                     // 高度（行数）
int cols = image.cols;                     // 宽度（列数）
int channels = image.channels();           // 通道数（彩色图=3，灰度图=1）
```

**三通道（BGR）**：彩色图每个像素有 3 个值——蓝(Blue)、绿(Green)、红(Red)，每个值 0~255。

**单通道（灰度）**：灰度图每个像素只有 1 个值——亮度，0 是纯黑，255 是纯白。

### 6.2 基本操作速查

```cpp
cv::cvtColor(color, gray, cv::COLOR_BGR2GRAY);  // 彩色转灰度
cv::GaussianBlur(src, dst, cv::Size(5,5), 0);   // 高斯模糊
cv::resize(src, dst, cv::Size(20, 40));          // 缩放
cv::rectangle(img, rect, cv::Scalar(0,255,0));   // 画矩形
cv::imwrite("output.jpg", image);                 // 保存图片
```

---

## 第 7 章：图像处理流水线（核心算法）

这是本项目最重要的部分。打开 `src/core/PlateDetector.cpp`，跟着下面的讲解逐函数理解。

### 7.1 整体流程

```
原始彩色图 → 灰度化 → 高斯模糊 → Sobel边缘检测 → 形态学处理 → 轮廓查找 → 车牌区域
```

为什么要这么多个步骤？因为直接对彩色图找车牌太困难了。每一步都是在"简化问题"：

1. **灰度化**：去掉颜色干扰，只保留亮度信息
2. **高斯模糊**：去掉噪声，防止噪声被误认为边缘
3. **Sobel**：找出亮度变化剧烈的位置（即边缘）
4. **形态学**：把相邻的边缘连成一片，形成完整的车牌区域
5. **轮廓查找**：从处理后的图像中找到封闭区域

### 7.2 灰度化 — toGrayscale()

```cpp
cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
```

为什么灰度化有用？因为边缘检测依赖的是亮度变化，而颜色信息对此帮助不大。灰度化把三维问题（RGB 三个通道）简化为一维问题（只有亮度），大幅降低计算量。

### 7.3 高斯模糊 — applyGaussianBlur()

```cpp
cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
```

想象一张照片上有很多细小噪点（比如 JPEG 压缩产生的像素杂色）。如果直接做边缘检测，这些噪点也会被认为是"边缘"。高斯模糊对图像做轻微平滑，消除这些噪点，同时保留真正的边缘（车牌边框、字符笔画）。

**参数调节**：`Size(5, 5)` 是模糊核大小。核越大，模糊效果越强，但也会丢失真实边缘细节。默认值 5 适合大多数场景。

### 7.4 Sobel 边缘检测 — applySobelEdge()

这是最关键的步骤之一。Sobel 算子通过计算图像亮度的梯度（变化率）来检测边缘。

```cpp
cv::Sobel(blurred, gradX, CV_16S, 1, 0, 3);  // x 方向梯度
cv::Sobel(blurred, gradY, CV_16S, 0, 1, 3);  // y 方向梯度
cv::addWeighted(absGradX, 0.7, absGradY, 0.3, 0, edges);
```

**x 方向 Sobel（dx=1, dy=0）**：检测水平方向上的亮度变化 → 找到垂直边缘。车牌上的字符（汉字、字母、数字）都有大量垂直笔画，所以 x 方向会产生强烈响应。

**y 方向 Sobel（dx=0, dy=1）**：检测垂直方向上的亮度变化 → 找到水平边缘。车牌的上下边框是水平边缘。

**为什么 x 方向权重更高（0.7 vs 0.3）？** 中国车牌的字符产生大量垂直边缘，这是车牌区域最显著的特征。提高 x 方向权重能让车牌区域更加突出。

### 7.5 形态学处理 — applyMorphology()

```cpp
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(17, 3));
cv::morphologyEx(edges, closed, cv::MORPH_CLOSE, kernel);
```

经过 Sobel 后，每个字符的每条笔画都是独立的边缘线段。"京" 字的点、横、竖是分开的碎片。我们需要把它们合并成一整块。

**闭运算（MORPH_CLOSE）**：先膨胀后腐蚀。膨胀会把相邻的边缘"扩张"直到连接在一起；腐蚀再"收缩"回去，但连接的部分不会断开。效果就是字符间的缝隙被填满了。

**矩形核 17×3**：这个核是宽而扁的（宽度 17 > 高度 3），因为车牌是横向排列的，字符间的间隔主要在水平方向。宽核能跨越字符间的水平间隙。

### 7.6 轮廓查找 — findPlateContours()

```cpp
cv::findContours(morphed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
```

`findContours` 在二值图像中找封闭轮廓。经过形态学处理后，车牌区域应该是一个大的封闭矩形。

但不是所有轮廓都是车牌——可能是车灯、格栅等。所以需要过滤：

- **面积过滤**：`minContourArea < area < maxContourArea`。车牌不会太小也不会太大。
- **宽高比过滤**：`minAspectRatio < width/height < maxAspectRatio`。中国蓝牌标准尺寸 440mm × 140mm，宽高比约 3.14。

### 7.7 动手实验

理解这些步骤最好的方式是**实际看每一步的输出图像**。你可以在 `detect()` 方法的每个步骤后添加：

```cpp
cv::imwrite("step1_gray.jpg", gray);
cv::imwrite("step2_blur.jpg", blurred);
cv::imwrite("step3_sobel.jpg", edges);
cv::imwrite("step4_morph.jpg", closed);
```

然后用一张车牌照片运行，对比每一步的效果图，你就能直观理解每个处理步骤在做什么。

---

## 第 8 章：字符分割与识别

### 8.1 字符分割 — segmentCharacters()

检测到车牌区域后，需要把 7 个字符（1 汉字 + 1 字母 + 5 字母数字）分开。

```cpp
cv::adaptiveThreshold(plateGray, binary, 255,
    cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 15, 5);
```

**自适应二值化**：把灰度图变成只有黑白两色。字符变黑（前景），背景变白（或反过来）。"自适应"的意思是每个像素根据自己周围的亮度决定阈值，比全局固定阈值更适应不均匀光照。

然后对二值图找轮廓，按面积和宽高比过滤出字符区域，按 x 坐标从左到右排序。

### 8.2 模板匹配 — matchCharacter()

```cpp
cv::matchTemplate(charImg, templateImg, result, cv::TM_CCOEFF_NORMED);
```

模板匹配的原理很简单：拿一张"标准字符图"（模板），和待识别的字符图逐像素比较相似度。`TM_CCOEFF_NORMED` 返回 -1 到 1 之间的相关系数，1 表示完全匹配。

我们把分割出的每个字符和所有模板逐一匹配，取最高得分的模板作为识别结果。

**局限性**：模板匹配对字符的旋转、缩放、变形很敏感。如果车牌照片拍摄角度倾斜，识别率会下降。这就是为什么深度学习方案（CRNN、YOLO）在实际应用中效果更好。

---

## 第 9 章：界面开发

### 9.1 主窗口：MainWindow

`MainWindow` 继承自 `QMainWindow`，是顶层窗口。它用 `QTabWidget` 创建三个标签页：

```cpp
m_tabWidget->addTab(m_recognitionWidget, "车牌识别");
m_tabWidget->addTab(m_historyWidget,     "识别记录");
m_tabWidget->addTab(m_statisticsWidget,  "数据统计");
```

菜单栏通过 `menuBar()->addMenu()` 和 `addAction()` 创建。

### 9.2 识别工作区：RecognitionWidget

这是最复杂的界面组件。左侧显示图片，右侧是控制面板。

**异步识别**是一个关键设计：图像处理可能耗时几百毫秒到几秒，如果在主线程执行，界面会卡住。所以用 `QtConcurrent::run` 把识别任务放到后台线程：

```cpp
QtConcurrent::run([this]() {
    auto result = m_recognizer->recognize(m_currentImage);
    QMetaObject::invokeMethod(this, [this, result]() {
        onRecognitionFinished(result);  // 回到主线程更新 UI
    }, Qt::QueuedConnection);
});
```

注意 `QMetaObject::invokeMethod` + `Qt::QueuedConnection`——它把结果安全地传回主线程。**永远不要在后台线程直接操作 UI 控件**。

### 9.3 摄像头：CameraWidget

Qt6 的多媒体 API 比 Qt5 简化了很多：

```cpp
QMediaCaptureSession *session = new QMediaCaptureSession();
QCamera *camera = new QCamera();
QVideoWidget *videoWidget = new QVideoWidget();

session->setCamera(camera);
session->setVideoOutput(videoWidget);
```

`QMediaDevices::videoInputs()` 枚举系统所有摄像头，用户可以在下拉框中选择。

### 9.4 历史记录：HistoryWidget

用 `QTableWidget` 显示表格数据。搜索和筛选的逻辑是：先从 RecordManager 获取所有记录，然后依次应用文本过滤、日期过滤、省份过滤，最后把结果填入表格。

**颜色编码**置信度：绿色（>80%）表示高可信，黄色（50-80%）表示需确认，红色（<50%）表示可能识别错误。

### 9.5 统计图表：StatisticsWidget

使用 Qt Charts 模块：

- **QPieSeries**：饼图，展示各省份在总识别记录中的占比
- **QBarSeries**：柱状图，展示每天的识别数量

Qt Charts 的基本用法：创建 Series → 添加数据 → 创建 QChart → 添加到 QChartView → 显示。

---

## 第 10 章：进阶方向

### 10.1 传统方法的局限

本项目的模板匹配方案有明显的局限：
- 对光照变化敏感（夜间、逆光效果差）
- 对倾斜角度敏感（需要车牌正对摄像头）
- 无法处理污损、遮挡的车牌
- 汉字识别率低（模板难以覆盖所有字体变化）

### 10.2 深度学习方案

现代车牌识别系统通常采用深度学习：

- **车牌检测**：YOLO / SSD 目标检测模型，直接定位车牌区域，比 Sobel+形态学更鲁棒
- **字符识别**：CRNN（CNN + RNN + CTC）端到端识别，不需要字符分割步骤
- **端到端**：LPRNet 等模型直接从车牌图片输出完整车牌号

OpenCV 的 `dnn` 模块可以加载训练好的模型进行推理。如果要升级本项目，可以替换 `PlateDetector` 和 `CharacterClassifier` 的实现，而 UI 层和数据层完全不需要改动——这就是分层架构的优势。

### 10.3 进一步学习资源

- Qt 官方文档：https://doc.qt.io/qt-6/
- OpenCV 官方教程：https://docs.opencv.org/4.x/
- Qt Charts 示例：Qt Creator → 欢迎 → 示例 → 搜索 "Charts"
- 车牌识别开源项目：HyperLPR（基于深度学习，支持中文车牌）

---

## 附录：代码阅读顺序建议

如果你是第一次看这个项目，建议按以下顺序打开文件：

| 顺序 | 文件 | 学习重点 |
|------|------|---------|
| 1 | main.cpp | 理解 Qt 程序入口和事件循环 |
| 2 | MainWindow.h/cpp | 理解 QMainWindow、Tab、菜单 |
| 3 | Record.h | 理解数据模型和 JSON 序列化 |
| 4 | RecordManager.cpp | 理解文件 I/O 和数据管理 |
| 5 | ImageConverter.cpp | 理解 Mat/QImage 转换 |
| 6 | PlateDetector.cpp | 理解图像处理流水线（最重要！） |
| 7 | CharacterClassifier.cpp | 理解字符分割和模板匹配 |
| 8 | PlateRecognizer.cpp | 理解模块协调和信号传递 |
| 9 | RecognitionWidget.cpp | 理解异步任务和 UI 更新 |
| 10 | HistoryWidget.cpp | 理解表格、搜索、筛选 |
| 11 | StatisticsWidget.cpp | 理解 Qt Charts |
| 12 | CameraWidget.cpp | 理解 Qt Multimedia |
| 13 | CMakeLists.txt | 理解构建配置 |
