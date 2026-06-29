# 车牌识别管理系统

基于 Qt6 + OpenCV + ONNX 深度学习的桌面端车牌自动识别系统。采用 YOLO 检测 + ONNX OCR 识别的双模型架构，辅以传统形态学兜底，实现高准确率的中国车牌识别。

> 本项目面向初学者设计，代码中包含大量中文注释。配套详细文档见 [docs/DOCUMENTATION.html](docs/DOCUMENTATION.html)。

---

## 技术栈

| 组件 | 版本 | 用途 |
|------|------|------|
| Qt | 6.11.1 (MSVC 2022 64-bit) | UI 框架 (Widgets) |
| OpenCV | 4.12.0 | 图像处理 + DNN 推理 |
| ONNX 模型 | - | YOLO 车牌检测 + OCR 文字识别 |
| Visual Studio | 2022 (v145) | C++17 编译 |
| Qt VS Tools | 最新版 | VS 集成 Qt 构建 |

---

## 识别流水线

```
输入图片 → [YOLO 车牌检测] → [ONNX OCR 识别] → [格式验证] → 输出车牌号
                ↓ (无结果)
          [形态学兜底检测]
```

1. **YOLO 检测** (car_plate_detect.onnx): 640x640 输入, 输出 bbox + 4角关键点 + 车牌类型
2. **形态学兜底**: Sobel 边缘 + HSV 蓝色区域, 当 YOLO 无结果时启用
3. **ONNX OCR** (plate_rec.onnx): 168x48 输入, 21 时间步 CTC 解码, 78 字符集
4. **格式验证**: 字符数 7-8 位 + 省份首字 + 字母次字 + 置信度 >= 0.85

---

## 功能概览

| 功能模块 | 说明 |
|---------|------|
| 车牌识别 | 从本地图片或摄像头拍照中检测并识别车牌 (蓝底/黄底/新能源) |
| 摄像头采集 | 支持系统摄像头实时预览与拍照 |
| 识别记录 | 保存每次识别结果, 支持按车牌号/日期/省份搜索筛选 |
| 数据统计 | 饼图/柱状图展示识别数据分布 |
| 数据导出 | 将记录导出为 CSV 文件 (Excel 兼容) |

---

## 环境要求

| 组件 | 版本 | 说明 |
|------|------|------|
| Windows | 10/11 64位 | 仅支持 Windows |
| Visual Studio | 2022 | 需安装 "C++ 桌面开发" + Qt VS Tools 扩展 |
| Qt | 6.11.1 (MSVC 2022 64-bit) | Multimedia 模块 (摄像头) |
| OpenCV | 4.12.0 | 预编译版本 (vc16) |

---

## 快速开始

### 1. 安装环境

1. **Visual Studio 2022**: 安装 "C++ 桌面开发" 工作负载 + Qt VS Tools 扩展
2. **Qt 6.11.1**: 通过 Qt Online Installer 安装 MSVC 2022 64-bit + Multimedia 模块
3. **OpenCV 4.12.0**: 下载预编译包解压到 `D:\opencv4\opencv`

> 如果 OpenCV 路径不同, 修改 `PlateRecognition.pro` 中的 `OPENCV_DIR` 和 `.vcxproj` 中的对应路径。

### 2. 下载模型

从 [ModelScope](https://modelscope.cn/models/supersong/onnxocr_model) 下载模型文件, 放入 `models/` 目录:

```
models/
├── car_plate_detect.onnx   ← YOLO 车牌检测 (5.3 MB)
└── plate_rec.onnx          ← OCR 文字识别 (649 KB)
```

### 3. 编译运行

**VS2022 + Qt VS Tools (推荐):**
打开 `PlateRecognition.vcxproj`, Ctrl+B 编译, F5 运行。

**qmake + MSVC:**
```cmd
# 在 x64 Native Tools Command Prompt 中
cd D:\Arctic_QT\PlateRecognition
qmake PlateRecognition.pro
nmake
```

Post-build 会自动拷贝 OpenCV DLL + ONNX 模型到 `bin/` 目录。

---

## 项目结构

```
PlateRecognition/
├── src/
│   ├── core/                          ← 核心算法
│   │   ├── PlateDetector.h/.cpp       ← YOLO + 形态学混合检测
│   │   ├── PlateOCR.h/.cpp           ← ONNX OCR + CTC 解码
│   │   └── PlateRecognizer.h/.cpp    ← 流水线主控 + 验证回退
│   ├── ui/                            ← 界面
│   │   ├── MainWindow                 ← 主窗口
│   │   ├── RecognitionWidget          ← 识别页
│   │   ├── HistoryWidget              ← 历史记录
│   │   ├── StatisticsWidget           ← 统计图表
│   │   └── CameraWidget               ← 摄像头
│   ├── data/                          ← 数据层
│   │   ├── Record.h                   ← 数据模型
│   │   └── RecordManager              ← JSON 持久化
│   └── utils/                         ← 工具
│       ├── CsvExporter                ← CSV 导出
│       └── ImageConverter             ← 格式转换
├── models/                            ← ONNX 模型文件
├── samples/                           ← 测试图片
├── docs/
│   └── DOCUMENTATION.html             ← 详细技术文档
├── PlateRecognition.pro               ← qmake 配置
└── PlateRecognition.vcxproj           ← VS 项目文件
```

---

## 已知限制

| 场景 | 表现 | 原因 |
|------|------|------|
| 蓝底车牌 + 蓝色车身 | YOLO 正确检测 ✅ | YOLO 可区分车身和车牌 |
| 纯车牌裁剪图 (无车身) | YOLO 失效, 形态学兜底 | YOLO 需要车身上下文 |
| 形近字混淆 (鲁/闽) | 偶尔误识别 | 模型训练数据有限 |
| 夜间/低光照 | 识别率下降 | 预处理未做光照补偿 |

---

## 致谢

ONNX 模型来自 [OnnxOCR](https://github.com/jingsongliujing/OnnxOCR) 项目, 通过 ModelScope 托管下载。
