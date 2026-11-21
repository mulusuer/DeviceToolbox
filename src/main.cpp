#include "mainwindow.h"

#include <QApplication>

// 功能：建主窗口并进入事件循环。
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
