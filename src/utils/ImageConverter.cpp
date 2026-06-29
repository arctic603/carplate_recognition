#include "utils/ImageConverter.h"
#include <opencv2/imgproc.hpp>

namespace ImageConverter {

QImage matToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }

    if (mat.type() == CV_8UC3) {
        // BGR \u8f6c RGB
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        // \u6df1\u62f7\u8d1d\uff0c\u786e\u4fdd\u6570\u636e\u72ec\u7acb
        return QImage(rgb.data, rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    }

    if (mat.type() == CV_8UC1) {
        // \u7070\u5ea6\u56fe\u76f4\u63a5\u8f6c\u6362\uff0c\u6df1\u62f7\u8d1d
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

    // \u7edf\u4e00\u8f6c\u6362\u4e3a ARGB32 \u683c\u5f0f
    QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    cv::Mat mat(argb.height(), argb.width(), CV_8UC4,
                const_cast<uchar*>(argb.bits()),
                static_cast<size_t>(argb.bytesPerLine()));

    // BGRA \u8f6c BGR\uff0c\u5e76\u6df1\u62f7\u8d1d
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
    return bgr.clone();
}

} // namespace ImageConverter
