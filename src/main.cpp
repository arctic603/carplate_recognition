#include "ui/MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置 Fusion 风格
    app.setStyle(QStyleFactory::create("Fusion"));

    // 设置应用信息
    app.setApplicationName(QString::fromUtf8(u8"\u8f66\u724c\u8bc6\u522b\u7ba1\u7406\u7cfb\u7edf"));
    app.setOrganizationName(QString::fromUtf8(u8"Arctic"));
    app.setApplicationVersion("1.0.0");

    MainWindow window;
    window.setWindowTitle(QString::fromUtf8(u8"\u8f66\u724c\u8bc6\u522b\u7ba1\u7406\u7cfb\u7edf"));
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
