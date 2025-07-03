//
// Created by wuxianggujun on 25-6-29.
//

#pragma once

#include <QtNodes/GraphicsView>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
#include <QtNodes/internal/NodeGraphicsObject.hpp>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMimeData>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QMenu>
#include <QObject>
#include <QLineF>
#include <QPen>
#include <QBrush>
#include <limits>

/**
 * @brief TinaFlow节点编辑器的图形视图
 * 
 * 继承自QtNodes::GraphicsView，增强了右键菜单功能：
 * - 支持节点右键菜单（删除、复制等）
 * - 支持连接线右键菜单（删除连接）
 * - 支持画布右键菜单（添加节点、清空画布等）
 */
class TinaFlowGraphicsView : public QtNodes::GraphicsView
{
    Q_OBJECT

public:
    explicit TinaFlowGraphicsView(QtNodes::DataFlowGraphicsScene* scene, QWidget* parent = nullptr)
        : QtNodes::GraphicsView(scene, parent)
        , m_scene(scene)
        , m_selectionRect(nullptr)
        , m_isSelecting(false)
        , m_isPanning(false)
    {
        // 启用拖拽接受
        setAcceptDrops(true);

        // 禁用默认的拖拽模式，我们要自定义
        setDragMode(QGraphicsView::NoDrag);

        qDebug() << "TinaFlowGraphicsView: Initialized with drag-drop support";
    }

protected:
    // 鼠标事件处理
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void contextMenuEvent(QContextMenuEvent* event) override
    {
        QPointF scenePos = mapToScene(event->pos());

        // 检查是否有多个选中的节点
        QList<QGraphicsItem*> selectedItems = scene()->selectedItems();
        QList<QtNodes::NodeId> selectedNodes;

        for (auto* item : selectedItems) {
            QtNodes::NodeId nodeId = findNodeAtItem(item);
            if (m_scene->graphModel().allNodeIds().contains(nodeId)) {
                selectedNodes.append(nodeId);
            }
        }

        // 如果有多个选中的节点，使用多选模式的节点菜单
        if (selectedNodes.size() > 1) {
            // 使用第一个节点的ID，但标记为多选模式
            emit nodeContextMenuRequested(selectedNodes.first(), scenePos, true);
            return;
        }

        // 检查鼠标位置下的图形项
        QGraphicsItem* item = m_scene->itemAt(scenePos, transform());
        if (!item) {
            emit sceneContextMenuRequested(scenePos);
            return;
        }

        // 检查是否是连接线
        if (auto* connectionObject = qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item)) {
            QtNodes::ConnectionId connectionId = findConnectionIdByGraphicsObject(connectionObject);
            emit connectionContextMenuRequested(connectionId, scenePos);
            return;
        }

        // 检查是否是节点
        QtNodes::NodeId nodeId = findNodeAtItem(item);
        if (m_scene->graphModel().allNodeIds().contains(nodeId)) {
            emit nodeContextMenuRequested(nodeId, scenePos, false);
            return;
        }

        // 默认情况：空白区域菜单
        emit sceneContextMenuRequested(scenePos);
    }


    
    void dragEnterEvent(QDragEnterEvent* event) override
    {
        // 检查是否为节点拖拽
        if (event->mimeData()->hasFormat("application/x-tinaflow-node")) {
            event->acceptProposedAction();
            qDebug() << "TinaFlowGraphicsView: Accepting node drag";
        } else {
            QtNodes::GraphicsView::dragEnterEvent(event);
        }
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        // 检查是否为节点拖拽
        if (event->mimeData()->hasFormat("application/x-tinaflow-node")) {
            event->acceptProposedAction();
        } else {
            QtNodes::GraphicsView::dragMoveEvent(event);
        }
    }

    void dropEvent(QDropEvent* event) override
    {
        // 检查是否为节点拖拽
        if (event->mimeData()->hasFormat("application/x-tinaflow-node")) {
            QString nodeType = QString::fromUtf8(event->mimeData()->data("application/x-tinaflow-node"));
            QPointF scenePos = mapToScene(event->pos());
            
            qDebug() << "TinaFlowGraphicsView: Dropping node" << nodeType << "at position" << scenePos;
            
            emit nodeCreationFromDragRequested(nodeType, scenePos);
            event->acceptProposedAction();
        } else {
            QtNodes::GraphicsView::dropEvent(event);
        }
    }

private:
    // 选择框相关方法
    void startSelection(const QPointF& startPos);
    void updateSelection(const QPointF& currentPos);
    void finishSelection();
    void clearSelection();

    // 视图平移相关方法
    void startPanning(const QPointF& startPos);
    void updatePanning(const QPointF& currentPos);
    void finishPanning();

    QtNodes::NodeId findNodeAtItem(QGraphicsItem* item)
    {
        // 向上查找节点图形对象
        QGraphicsItem* current = item;
        int depth = 0;
        while (current && depth < 10) {  // 防止无限循环
            if (auto* nodeObject = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(current)) {
                return nodeObject->nodeId();
            }
            current = current->parentItem();
            depth++;
        }

        return QtNodes::NodeId{};
    }

    QtNodes::ConnectionId findConnectionIdByGraphicsObject(QtNodes::ConnectionGraphicsObject* connectionObject)
    {
        // 遍历所有连接，找到对应的图形对象
        auto allNodes = m_scene->graphModel().allNodeIds();
        for (auto nodeId : allNodes) {
            auto connections = m_scene->graphModel().allConnectionIds(nodeId);
            for (auto connectionId : connections) {
                // 通过场景获取对应的图形对象
                if (auto* sceneConnectionObject = m_scene->connectionGraphicsObject(connectionId)) {
                    if (sceneConnectionObject == connectionObject) {
                        return connectionId;
                    }
                }
            }
        }

        // 如果没找到，返回无效的ConnectionId
        return QtNodes::ConnectionId{};
    }

signals:
    void nodeContextMenuRequested(QtNodes::NodeId nodeId, const QPointF& pos, bool isMultiSelection = false);
    void connectionContextMenuRequested(QtNodes::ConnectionId connectionId, const QPointF& pos);
    void sceneContextMenuRequested(const QPointF& pos);
    void nodeCreationFromDragRequested(const QString& nodeType, const QPointF& position);

private:
    QtNodes::DataFlowGraphicsScene* m_scene;

    // 选择框相关
    QGraphicsRectItem* m_selectionRect;
    bool m_isSelecting;
    QPointF m_selectionStartPos;

    // 视图平移相关
    bool m_isPanning;
    QPointF m_panStartPos;
    QPointF m_panLastPos;
};
