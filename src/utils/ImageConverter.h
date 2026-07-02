#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <QImage>
#include <opencv2/core/mat.hpp>

/**
 * @brief 图像格式转换工具命名空间
 *
 * 提供 cv::Mat 与 QImage 之间的双向转换，均为深拷贝
 */
namespace ImageConverter {

/**
 * @brief 将 OpenCV Mat 转换为 QImage
 * @param mat 输入的 OpenCV 图像，支持 CV_8UC3 (BGR) 和 CV_8UC1 (灰度)
 * @return 转换后的 QImage（深拷贝）
 */
QImage matToQImage(const cv::Mat &mat);

/**
 * @brief 将 QImage 转换为 OpenCV Mat
 * @param image 输入的 QImage
 * @return 转换后的 cv::Mat (BGR 格式，深拷贝)
 */
cv::Mat qImageToMat(const QImage &image);

} // namespace ImageConverter

#endif // IMAGECONVERTER_H
