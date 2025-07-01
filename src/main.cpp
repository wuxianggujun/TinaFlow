#include <QApplication>
#include <QStyleFactory>
#include <QMessageBox>
#include <QDebug>
#include <exception>
#include "mainwindow.hpp"

// 全局异常处理器
void handleException(const std::exception& e) {
    qCritical() << "Unhandled exception:" << e.what();
    QMessageBox::critical(nullptr, "严重错误",
        QString("程序遇到未处理的异常：\n%1\n\n程序将尝试继续运行，但可能不稳定。").arg(e.what()));
}

void handleUnknownException() {
    qCritical() << "Unknown exception caught";
    QMessageBox::critical(nullptr, "严重错误",
        "程序遇到未知异常。\n\n程序将尝试继续运行，但可能不稳定。");
}

int main(int argc, char* argv[])
{
#if defined(Q_OS_WINDOWS)
    // 修复Qt6在Windows 11上QComboBox在QGraphicsProxyWidget中无法显示下拉菜单的问题
    // 参考: https://forum.qt.io/topic/158614/qcombobox-fails-to-display-dropdown-when-placed-in-qgraphicsview-via-qgraphicsproxywidget
    // 这是目前最可靠的解决方案
    QApplication::setStyle(QStyleFactory::create("windowsvista"));
#endif

    QApplication app(argc, argv);

    // 设置应用程序信息
    QApplication::setApplicationName("TinaFlow");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("TinaFlow Team");

    // 设置全局异常处理
    std::set_terminate([]() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            handleException(e);
        } catch (...) {
            handleUnknownException();
        }
        std::abort();
    });

    try {
        MainWindow w;
        w.show();
        return QApplication::exec();
    } catch (const std::exception& e) {
        handleException(e);
        return -1;
    } catch (...) {
        handleUnknownException();
        return -1;
    }
}
