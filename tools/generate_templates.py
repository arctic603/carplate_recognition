# -*- coding: utf-8 -*-
"""
车牌识别字符模板生成工具

用途：
    使用 OpenCV 生成字母 (A-Z)、数字 (0-9) 以及省份占位模板图片，
    供 CharacterClassifier 进行模板匹配时使用。

用法：
    python tools/generate_templates.py
    python tools/generate_templates.py --output-dir resources/templates

生成目录结构：
    <output-dir>/
        provinces/   - 省份占位模板 (拼音命名)
        letters/     - 字母模板 A-Z + 数字 0-9
        alphanum/    - 字母+数字组合模板 (0-9, A-Z)
"""

import os
import argparse
import cv2
import numpy as np


# ---------- 配置 ----------
CANVAS_W = 20
CANVAS_H = 40
FONT = cv2.FONT_HERSHEY_SIMPLEX
FONT_SCALE = 0.7
COLOR_BLACK = (0, 0, 0)
THICKNESS = 2

# 省份列表：汉字 -> 拼音文件名
PROVINCES = {
    "jing":  "京",
    "hu":    "沪",
    "jin":   "津",
    "yu":    "渝",
    "ji":    "冀",
    "yu2":   "豫",
    "yun":   "云",
    "liao":  "辽",
    "hei":   "黑",
    "xiang": "湘",
    "wan":   "皖",
    "lu":    "鲁",
    "su":    "苏",
    "zhe":   "浙",
    "gan":   "赣",
    "e":     "鄂",
    "yue":   "粤",
    "chuan": "川",
    "shan":  "陕",
    "min":   "闽",
}


def make_char_image(char: str) -> np.ndarray:
    """在白色画布上居中绘制单个 ASCII 字符，返回图像矩阵。"""
    canvas = np.ones((CANVAS_H, CANVAS_W), dtype=np.uint8) * 255
    # 先获取文字尺寸以计算居中位置
    (text_w, text_h), baseline = cv2.getTextSize(
        char, FONT, FONT_SCALE, THICKNESS
    )
    x = (CANVAS_W - text_w) // 2
    y = (CANVAS_H + text_h) // 2
    cv2.putText(canvas, char, (x, y), FONT, FONT_SCALE, COLOR_BLACK, THICKNESS)
    return canvas


def make_province_placeholder(index: int) -> np.ndarray:
    """
    生成省份占位模板。
    由于 cv2.putText 不支持中文，使用编号 P{index} 加矩形方块作为占位。
    """
    canvas = np.ones((CANVAS_H, CANVAS_W), dtype=np.uint8) * 255
    # 画一个居中的灰色方块作为占位符
    margin_x = 3
    margin_y = 6
    cv2.rectangle(
        canvas, (margin_x, margin_y),
        (CANVAS_W - margin_x, CANVAS_H - margin_y),
        128, -1
    )
    # 在方块上方叠加编号
    label = f"P{index}"
    (text_w, text_h), _ = cv2.getTextSize(label, FONT, 0.5, 1)
    tx = (CANVAS_W - text_w) // 2
    ty = (CANVAS_H + text_h) // 2
    cv2.putText(canvas, label, (tx, ty), FONT, 0.5, 0, 1)
    return canvas


def generate_all(output_dir: str) -> None:
    """生成所有模板图片到 output_dir 下对应的子目录。"""
    provinces_dir = os.path.join(output_dir, "provinces")
    letters_dir = os.path.join(output_dir, "letters")
    alphanum_dir = os.path.join(output_dir, "alphanum")

    for d in (provinces_dir, letters_dir, alphanum_dir):
        os.makedirs(d, exist_ok=True)

    # --- 1. 字母模板 A-Z ---
    for i in range(26):
        char = chr(ord("A") + i)
        img = make_char_image(char)
        cv2.imwrite(os.path.join(letters_dir, f"{char}.png"), img)
    print(f"[OK] 字母模板 A-Z -> {letters_dir}")

    # --- 2. 数字模板 0-9 (写入 letters/ 和 alphanum/) ---
    for i in range(10):
        char = str(i)
        img = make_char_image(char)
        cv2.imwrite(os.path.join(letters_dir, f"{char}.png"), img)
        cv2.imwrite(os.path.join(alphanum_dir, f"{char}.png"), img)
    print(f"[OK] 数字模板 0-9 -> {letters_dir} + {alphanum_dir}")

    # --- 3. 字母也写入 alphanum/ ---
    for i in range(26):
        char = chr(ord("A") + i)
        img = make_char_image(char)
        cv2.imwrite(os.path.join(alphanum_dir, f"{char}.png"), img)
    print(f"[OK] 字母模板 A-Z -> {alphanum_dir}")

    # --- 4. 省份占位模板 ---
    for idx, (pinyin, _hanzi) in enumerate(PROVINCES.items()):
        img = make_province_placeholder(idx)
        cv2.imwrite(os.path.join(provinces_dir, f"{pinyin}.png"), img)
    print(f"[OK] 省份占位模板 ({len(PROVINCES)} 个) -> {provinces_dir}")

    total = 26 + 10 + 26 + len(PROVINCES)
    print(f"\n共生成 {total} 张模板图片，输出目录: {output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="车牌识别字符模板生成工具"
    )
    parser.add_argument(
        "--output-dir",
        default="resources/templates",
        help="模板输出根目录 (默认: resources/templates)",
    )
    args = parser.parse_args()
    generate_all(args.output_dir)


if __name__ == "__main__":
    main()
