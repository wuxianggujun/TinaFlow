//
// Created by wuxianggujun on 25-6-29.
//

#pragma once

#include <QtNodes/GraphicsView>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/internal/ConnectionGraphicsObject.hpp>
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
        // 获取鼠标位置下的图形项
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem* item = m_scene->itemAt(scenePos, transform());

        if (!item) {
            // 空白区域右键菜单
            emit sceneContextMenuRequested(scenePos);
            return;
        }

        // 检查是否是节点
        if (auto* nodeItem = findNodeItem(item)) {
            // 获取节点ID
            QtNodes::NodeId nodeId = getNodeIdFromItem(nodeItem);
            if (nodeId != QtNodes::NodeId{}) {
                emit nodeContextMenuRequested(nodeId, scenePos);
                return;
            }
        }

        // 检查是否是连接线 - 使用场景遍历的方法
        if (auto* connectionObject = qgraphicsitem_cast<QtNodes::ConnectionGraphicsObject*>(item)) {
            // 通过遍历场景中的连接来找到对应的ConnectionId
            QtNodes::ConnectionId connectionId = findConnectionIdByGraphicsObject(connectionObject);
            emit connectionContextMenuRequested(connectionId, scenePos);
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
    QGraphicsItem* findNodeItem(QGraphicsItem* item)
    {
        // 向上查找，直到找到节点项或到达顶层
        QGraphicsItem* current = item;
        while (current) {
            // 使用type()方法来判断图形项类型
            // QtNodes的节点通常有特定的type值
            int itemType = current->type();

            // 检查是否是QObject派生类，如果是则可以使用metaObject
            if (auto* object = dynamic_cast<QObject*>(current)) {
                QString typeName = object->metaObject()->className();
                if (typeName.contains("Node") && typeName.contains("Graphics")) {
                    return current;
                }
            }

            // 也可以通过类型ID来判断（QtNodes可能使用自定义类型）
            if (itemType > QGraphicsItem::UserType) {
                // 这可能是一个自定义的图形项，假设是节点
                return current;
            }

            current = current->parentItem();
        }
        return nullptr;
    }
    
    // 不再需要复杂的连接线查找逻辑，直接使用qgraphicsitem_cast
    
    QtNodes::NodeId getNodeIdFromItem(QGraphicsItem* nodeItem)
    {
        // 这里需要通过场景来获取节点ID
        // QtNodes库的内部实现可能有所不同
        // 我们可以通过场景的方法来查找
        
        // 遍历所有节点，找到对应的图形项
        auto allNodes = m_scene->graphModel().allNodeIds();
        for (auto nodeId : allNodes) {
            // 这里需要QtNodes库提供的方法来获取节点的图形项
            // 由于API限制，我们使用一个简化的方法
            
            // 临时解决方案：通过位置来匹配
            QPointF itemPos = nodeItem->scenePos();
            QVariant nodePos = m_scene->graphModel().nodeData(nodeId, QtNodes::NodeRole::Position);
            if (nodePos.isValid()) {
                QPointF nodePosF = nodePos.toPointF();
                // 如果位置接近，认为是同一个节点
                if ((itemPos - nodePosF).manhattanLength() < 10) {
                    return nodeId;
                }
            }
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
    void nodeContextMenuRequested(QtNodes::NodeId nodeId, const QPointF& pos);
    void connectionContextMenuRequested(QtNodes::ConnectionId connectionId, const QPointF& pos);
    void sceneContextMenuRequested(const QPointF& pos);
    void nodeCreationFromDragRequested(const QString& nodeType, const QPointF& position);

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
};
