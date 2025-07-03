//
// Created by wuxianggujun on 25-7-3.
//

#include "TinaFlowGraphicsView.hpp"
#include <QDebug>
#include <QApplication>
#include <QScrollBar>

void TinaFlowGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());

        // 检查是否点击在节点或连接线上
        QGraphicsItem* item = m_scene->itemAt(scenePos, transform());

        // 如果点击在空白区域，开始选择框
        if (!item || (!qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item) &&
                      !qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item))) {
            startSelection(scenePos);
            return;
        }

        // 如果点击在节点或连接线上，传递给基类处理
        QtNodes::GraphicsView::mousePressEvent(event);
    }
    else if (event->button() == Qt::MiddleButton) {
        // 中键开始平移 - 使用视图坐标
        startPanning(event->pos());
        return;
    }
    else {
        // 其他按键传递给基类
        QtNodes::GraphicsView::mousePressEvent(event);
    }
}

void TinaFlowGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isSelecting) {
        // 更新选择框 - 使用场景坐标
        QPointF scenePos = mapToScene(event->pos());
        updateSelection(scenePos);
        return;
    }
    else if (m_isPanning) {
        // 更新视图平移 - 使用视图坐标
        updatePanning(event->pos());
        return;
    }

    // 传递给基类处理
    QtNodes::GraphicsView::mouseMoveEvent(event);
}

void TinaFlowGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        // 完成选择
        finishSelection();
        return;
    }
    else if (event->button() == Qt::MiddleButton && m_isPanning) {
        // 完成平移
        finishPanning();
        return;
    }
    
    // 传递给基类处理
    QtNodes::GraphicsView::mouseReleaseEvent(event);
}

void TinaFlowGraphicsView::startSelection(const QPointF& startPos)
{
    m_isSelecting = true;
    m_selectionStartPos = startPos;
    
    // 创建选择框
    if (!m_selectionRect) {
        m_selectionRect = new QGraphicsRectItem();
        QPen pen(QColor(0, 120, 215), 1, Qt::DashLine);
        QBrush brush(QColor(0, 120, 215, 30));
        m_selectionRect->setPen(pen);
        m_selectionRect->setBrush(brush);
        m_selectionRect->setZValue(1000); // 确保在最上层
        m_scene->addItem(m_selectionRect);
    }
    
    // 设置初始矩形
    m_selectionRect->setRect(QRectF(startPos, startPos));
    m_selectionRect->setVisible(true);
    
    // 清除之前的选择
    m_scene->clearSelection();
}

void TinaFlowGraphicsView::updateSelection(const QPointF& currentPos)
{
    if (!m_isSelecting || !m_selectionRect) return;
    
    // 计算选择矩形
    QRectF rect(m_selectionStartPos, currentPos);
    rect = rect.normalized(); // 确保宽高为正
    
    // 更新选择框
    m_selectionRect->setRect(rect);
    
    // 选择矩形内的节点
    m_scene->clearSelection();
    QList<QGraphicsItem*> itemsInRect = m_scene->items(rect, Qt::IntersectsItemShape);
    
    for (auto* item : itemsInRect) {
        if (auto* nodeObject = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item)) {
            nodeObject->setSelected(true);
        }
    }
}

void TinaFlowGraphicsView::finishSelection()
{
    m_isSelecting = false;
    
    // 隐藏选择框
    if (m_selectionRect) {
        m_selectionRect->setVisible(false);
    }
}

void TinaFlowGraphicsView::clearSelection()
{
    if (m_selectionRect) {
        m_scene->removeItem(m_selectionRect);
        delete m_selectionRect;
        m_selectionRect = nullptr;
    }
    m_isSelecting = false;
}

void TinaFlowGraphicsView::startPanning(const QPointF& startPos)
{
    m_isPanning = true;
    m_panStartPos = startPos;
    m_panLastPos = startPos;

    // 设置鼠标光标为手型
    setCursor(Qt::ClosedHandCursor);
}

void TinaFlowGraphicsView::updatePanning(const QPointF& currentPos)
{
    if (!m_isPanning) return;

    // 计算移动距离（使用视图坐标）
    QPointF delta = currentPos - m_panLastPos;
    m_panLastPos = currentPos;

    // 移动视图（注意方向相反）
    QScrollBar* hBar = horizontalScrollBar();
    QScrollBar* vBar = verticalScrollBar();

    // 使用整数值避免浮点误差
    hBar->setValue(hBar->value() - static_cast<int>(delta.x()));
    vBar->setValue(vBar->value() - static_cast<int>(delta.y()));
}

void TinaFlowGraphicsView::finishPanning()
{
    m_isPanning = false;
    
    // 恢复鼠标光标
    setCursor(Qt::ArrowCursor);
}
