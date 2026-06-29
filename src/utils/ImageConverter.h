#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <QImage>
#include <opencv2/core/mat.hpp>

/**
 * @brief \u56fe\u50cf\u683c\u5f0f\u8f6c\u6362\u5de5\u5177\u547d\u540d\u7a7a\u95f4
 *
 * \u63d0\u4f9b cv::Mat \u4e0e QImage \u4e4b\u95f4\u7684\u53cc\u5411\u8f6c\u6362\uff0c\u5747\u4e3a\u6df1\u62f7\u8d1d
 */
namespace ImageConverter {

/**
 * @brief \u5c06 OpenCV Mat \u8f6c\u6362\u4e3a QImage
 * @param mat \u8f93\u5165\u7684 OpenCV \u56fe\u50cf\uff0c\u652f\u6301 CV_8UC3 (BGR) \u548c CV_8UC1 (\u7070\u5ea6)
 * @return \u8f6c\u6362\u540e\u7684 QImage\uff08\u6df1\u62f7\u8d1d\uff09
 */
QImage matToQImage(const cv::Mat &mat);

/**
 * @brief \u5c06 QImage \u8f6c\u6362\u4e3a OpenCV Mat
 * @param image \u8f93\u5165\u7684 QImage
 * @return \u8f6c\u6362\u540e\u7684 cv::Mat (BGR \u683c\u5f0f\uff0c\u6df1\u62f7\u8d1d)
 */
cv::Mat qImageToMat(const QImage &image);

} // namespace ImageConverter

#endif // IMAGECONVERTER_H
