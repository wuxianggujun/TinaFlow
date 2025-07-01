//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include <QComboBox>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QAbstractItemView>
#include <QDebug>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>

/**
 * @brief 自定义ComboBox类，解决在QGraphicsView中下拉菜单定位问题
 * 
 * 在QGraphicsView/QGraphicsScene体系中使用QComboBox时，其下拉菜单可能会遇到
 * 坐标转换和定位问题。这个自定义类通过重写showPopup方法来正确处理坐标映射。
 */
class CustomComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit CustomComboBox(QWidget* parent = nullptr) : QComboBox(parent) 
    {
        // 设置一些基本属性以确保在QGraphicsProxyWidget中正常工作
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_MacShowFocusRect, false);
    }

protected:
    /**
     * @brief 重写showPopup方法，正确处理QGraphicsView中的坐标转换
     */
    void showPopup() override
    {
        qDebug() << "CustomComboBox::showPopup called";
        
        // 获取当前的QGraphicsProxyWidget
        QGraphicsProxyWidget* proxy = graphicsProxyWidget();
        if (!proxy) {
            qDebug() << "CustomComboBox: No proxy widget, using default showPopup";
            QComboBox::showPopup();
            return;
        }

        // 获取场景和视图
        QGraphicsScene* scene = proxy->scene();
        if (!scene || scene->views().isEmpty()) {
            qDebug() << "CustomComboBox: No scene or views, using default showPopup";
            QComboBox::showPopup();
            return;
        }

        QGraphicsView* view = scene->views().first();
        if (!view) {
            qDebug() << "CustomComboBox: No view, using default showPopup";
            QComboBox::showPopup();
            return;
        }

        // 先调用默认的showPopup来创建弹出窗口
        QComboBox::showPopup();

        // 获取弹出窗口
        QAbstractItemView* popup = this->view();
        if (!popup || !popup->window()) {
            qDebug() << "CustomComboBox: No popup window found";
            return;
        }

        QWidget* popupWindow = popup->window();

        // 确保弹出窗口有正确的窗口标志
        popupWindow->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
        popupWindow->setAttribute(Qt::WA_ShowWithoutActivating, false);

        qDebug() << "CustomComboBox: Popup window geometry before:" << popupWindow->geometry();
        qDebug() << "CustomComboBox: Popup window visible:" << popupWindow->isVisible();

        // 计算合适的弹出窗口大小
        int itemCount = this->count();
        int itemHeight = popup->sizeHintForRow(0);
        if (itemHeight <= 0) itemHeight = 20; // 默认行高

        int popupHeight = itemCount * itemHeight + 4; // 加一些边距
        int popupWidth = this->width();
        if (popupWidth < 150) popupWidth = 150; // 最小宽度

        qDebug() << "CustomComboBox: Calculated popup size:" << popupWidth << "x" << popupHeight;
        qDebug() << "CustomComboBox: Item count:" << itemCount << ", Item height:" << itemHeight;

        try {
            // 计算ComboBox在场景中的位置
            QPointF comboScenePos = proxy->scenePos();
            QRectF comboRect = proxy->boundingRect();
            
            // 计算弹出菜单应该显示的场景位置（ComboBox下方）
            QPointF popupScenePos = comboScenePos + QPointF(0, comboRect.height());
            
            // 将场景坐标转换为视图坐标
            QPoint popupViewPos = view->mapFromScene(popupScenePos);
            
            // 将视图坐标转换为全局屏幕坐标
            QPoint popupGlobalPos = view->mapToGlobal(popupViewPos);
            
            qDebug() << "CustomComboBox: Combo scene pos:" << comboScenePos;
            qDebug() << "CustomComboBox: Popup scene pos:" << popupScenePos;
            qDebug() << "CustomComboBox: Popup view pos:" << popupViewPos;
            qDebug() << "CustomComboBox: Popup global pos:" << popupGlobalPos;
            
            // 设置弹出窗口的位置
            popupWindow->move(popupGlobalPos);
            
            // 确保弹出窗口在屏幕范围内
            QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
            QRect popupGeometry = popupWindow->geometry();
            
            // 如果弹出窗口超出屏幕右边界，向左调整
            if (popupGeometry.right() > screenGeometry.right()) {
                popupGlobalPos.setX(screenGeometry.right() - popupGeometry.width());
            }
            
            // 如果弹出窗口超出屏幕下边界，显示在ComboBox上方
            if (popupGlobalPos.y() + popupHeight > screenGeometry.bottom()) {
                QPointF popupScenePosAbove = comboScenePos - QPointF(0, popupHeight);
                QPoint popupViewPosAbove = view->mapFromScene(popupScenePosAbove);
                QPoint popupGlobalPosAbove = view->mapToGlobal(popupViewPosAbove);
                popupGlobalPos = popupGlobalPosAbove;
            }

            // 设置弹出窗口的大小和位置
            popupWindow->setGeometry(popupGlobalPos.x(), popupGlobalPos.y(), popupWidth, popupHeight);
            popupWindow->raise();
            popupWindow->activateWindow();
            popupWindow->show();

            qDebug() << "CustomComboBox: Final popup position:" << popupGlobalPos;
            qDebug() << "CustomComboBox: Popup window geometry after:" << popupWindow->geometry();
            qDebug() << "CustomComboBox: Popup window visible after:" << popupWindow->isVisible();
            
        } catch (const std::exception& e) {
            qDebug() << "CustomComboBox: Error in coordinate transformation:" << e.what();
        }
    }

private:
    /**
     * @brief 获取当前widget的QGraphicsProxyWidget
     */
    QGraphicsProxyWidget* graphicsProxyWidget() const
    {
        QWidget* w = const_cast<CustomComboBox*>(this);
        while (w) {
            if (auto* proxy = qobject_cast<QGraphicsProxyWidget*>(w->graphicsProxyWidget())) {
                return proxy;
            }
            w = w->parentWidget();
        }
        return nullptr;
    }
};
