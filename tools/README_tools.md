# 工具脚本说明

## generate_templates.py

使用 OpenCV 生成车牌识别所需的字符模板图片（字母、数字、省份占位符）。

### 运行方式

```bash
# 默认输出到 resources/templates/
python tools/generate_templates.py

# 指定输出目录
python tools/generate_templates.py --output-dir resources/templates
```

### 生成的目录结构

```
resources/templates/
    provinces/   # 省份占位模板 (拼音命名，如 jing.png, hu.png)
    letters/     # 字母 A-Z + 数字 0-9
    alphanum/    # 字母+数字组合 (0-9, A-Z)，共 36 个字符
```

### 依赖

- Python 3.x
- OpenCV (`pip install opencv-python`)
- NumPy (`pip install numpy`)
