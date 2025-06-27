#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.hpp"

int main(int argc, char* argv[])
{
#if defined(Q_OS_WINDOWS)
    // 修复Qt6在Windows 11上QComboBox在QGraphicsProxyWidget中无法显示下拉菜单的问题
    // 参考: https://forum.qt.io/topic/158614/qcombobox-fails-to-display-dropdown-when-placed-in-qgraphicsview-via-qgraphicsproxywidget
    // 这是目前最可靠的解决方案
    QApplication::setStyle(QStyleFactory::create("windowsvista"));
#endif

    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
}
