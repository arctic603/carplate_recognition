#include "utils/ImageConverter.h"
#include <opencv2/imgproc.hpp>

namespace ImageConverter {

QImage matToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }

    if (mat.type() == CV_8UC3) {
        // BGR 转 RGB
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        // 深拷贝，确保数据独立
        return QImage(rgb.data, rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    }

    if (mat.type() == CV_8UC1) {
        // 灰度图直接转换，深拷贝
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step),
                      QImage::Format_Grayscale8).copy();
    }

    return QImage();
}

cv::Mat qImageToMat(const QImage &image)
{
    if (image.isNull()) {
        return cv::Mat();
    }

    // 统一转换为 ARGB32 格式
    QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    cv::Mat mat(argb.height(), argb.width(), CV_8UC4,
                const_cast<uchar*>(argb.bits()),
                static_cast<size_t>(argb.bytesPerLine()));

    // BGRA 转 BGR，并深拷贝
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
    return bgr.clone();
}

} // namespace ImageConverter
