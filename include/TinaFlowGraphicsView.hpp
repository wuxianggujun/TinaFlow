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
#include <QMimeData>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QMenu>
#include <QObject>
#include <QLineF>
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
    {
        // 启用拖拽接受
        setAcceptDrops(true);
        qDebug() << "TinaFlowGraphicsView: Initialized with drag-drop support";
    }

protected:
    void contextMenuEvent(QContextMenuEvent* event) override
    {
        QPointF scenePos = mapToScene(event->pos());
        qDebug() << "TinaFlowGraphicsView: Context menu at" << scenePos;

        // 检查鼠标位置下的图形项
        QGraphicsItem* item = m_scene->itemAt(scenePos, transform());
        if (!item) {
            qDebug() << "TinaFlowGraphicsView: No item found, showing scene menu";
            emit sceneContextMenuRequested(scenePos);
            return;
        }

        // 检查是否是连接线
        if (auto* connectionObject = qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item)) {
            qDebug() << "TinaFlowGraphicsView: Found connection item";
            QtNodes::ConnectionId connectionId = findConnectionIdByGraphicsObject(connectionObject);
            emit connectionContextMenuRequested(connectionId, scenePos);
            return;
        }

        // 检查是否是节点
        QtNodes::NodeId nodeId = findNodeAtItem(item);
        auto allNodes = m_scene->graphModel().allNodeIds();
        bool isValidNode = allNodes.contains(nodeId);

        qDebug() << "TinaFlowGraphicsView: Found nodeId:" << nodeId << "isValid:" << isValidNode;

        if (isValidNode) {
            qDebug() << "TinaFlowGraphicsView: Found node item, nodeId:" << nodeId;
            emit nodeContextMenuRequested(nodeId, scenePos);
            return;
        }

        // 默认情况：空白区域菜单
        qDebug() << "TinaFlowGraphicsView: Showing scene menu as fallback";
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
    QtNodes::NodeId findNodeAtItem(QGraphicsItem* item)
    {
        // 输出项的类型信息
        if (auto* object = dynamic_cast<QObject*>(item)) {
            qDebug() << "TinaFlowGraphicsView: Item type:" << object->metaObject()->className();
        }

        // 向上查找节点图形对象
        QGraphicsItem* current = item;
        int depth = 0;
        while (current && depth < 10) {  // 防止无限循环
            if (auto* object = dynamic_cast<QObject*>(current)) {
                qDebug() << "TinaFlowGraphicsView: Checking depth" << depth << "type:" << object->metaObject()->className();
            }

            if (auto* nodeObject = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(current)) {
                QtNodes::NodeId nodeId = nodeObject->nodeId();
                qDebug() << "TinaFlowGraphicsView: Found NodeGraphicsObject at depth" << depth << "nodeId:" << nodeId;
                return nodeId;
            }
            current = current->parentItem();
            depth++;
        }

        qDebug() << "TinaFlowGraphicsView: No NodeGraphicsObject found in hierarchy";
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
    void nodeContextMenuRequested(QtNodes::NodeId nodeId, const QPointF& pos);
    void connectionContextMenuRequested(QtNodes::ConnectionId connectionId, const QPointF& pos);
    void sceneContextMenuRequested(const QPointF& pos);
    void nodeCreationFromDragRequested(const QString& nodeType, const QPointF& position);

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
};
