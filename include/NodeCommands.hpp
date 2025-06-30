//
// Created by TinaFlow Team
//

#pragma once

#include "Command.hpp"
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/ConnectionStyle>
#include <QPointF>
#include <QJsonObject>

// 前向声明
namespace QtNodes {
    class Node;
    class Connection;
}

/**
 * @brief 创建节点命令
 */
class CreateNodeCommand : public Command
{
public:
    CreateNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                     const QString& nodeType,
                     const QPointF& position);
    
    bool execute() override;
    bool undo() override;
    bool redo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    
    // 获取创建的节点ID（执行后可用）
    QtNodes::NodeId getNodeId() const { return m_nodeId; }

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QString m_nodeType;
    QPointF m_position;
    QtNodes::NodeId m_nodeId;
    QJsonObject m_nodeData;  // 保存节点数据用于撤销
};

/**
 * @brief 删除节点命令
 */
class DeleteNodeCommand : public Command
{
public:
    DeleteNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                     QtNodes::NodeId nodeId);
    
    bool execute() override;
    bool undo() override;
    bool redo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QtNodes::NodeId m_nodeId;
    QString m_nodeType;
    QPointF m_position;
    QJsonObject m_nodeData;
    QList<QtNodes::ConnectionId> m_connections;  // 保存相关连接
};

/**
 * @brief 移动节点命令
 */
class MoveNodeCommand : public Command
{
public:
    MoveNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                   QtNodes::NodeId nodeId,
                   const QPointF& oldPosition,
                   const QPointF& newPosition);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    bool canMergeWith(const Command* other) const override;
    bool mergeWith(const Command* other) override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QtNodes::NodeId m_nodeId;
    QPointF m_oldPosition;
    QPointF m_newPosition;
};

/**
 * @brief 创建连接命令
 */
class CreateConnectionCommand : public Command
{
public:
    CreateConnectionCommand(QtNodes::DataFlowGraphicsScene* scene,
                           QtNodes::NodeId outputNodeId,
                           QtNodes::PortIndex outputPortIndex,
                           QtNodes::NodeId inputNodeId,
                           QtNodes::PortIndex inputPortIndex);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    
    // 获取创建的连接ID（执行后可用）
    QtNodes::ConnectionId getConnectionId() const { return m_connectionId; }

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QtNodes::NodeId m_outputNodeId;
    QtNodes::PortIndex m_outputPortIndex;
    QtNodes::NodeId m_inputNodeId;
    QtNodes::PortIndex m_inputPortIndex;
    QtNodes::ConnectionId m_connectionId;
};

/**
 * @brief 删除连接命令
 */
class DeleteConnectionCommand : public Command
{
public:
    DeleteConnectionCommand(QtNodes::DataFlowGraphicsScene* scene,
                           QtNodes::ConnectionId connectionId);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QtNodes::ConnectionId m_connectionId;
    QtNodes::NodeId m_outputNodeId;
    QtNodes::PortIndex m_outputPortIndex;
    QtNodes::NodeId m_inputNodeId;
    QtNodes::PortIndex m_inputPortIndex;
};

/**
 * @brief 修改节点属性命令
 */
class ModifyNodePropertyCommand : public Command
{
public:
    ModifyNodePropertyCommand(QtNodes::DataFlowGraphicsScene* scene,
                             QtNodes::NodeId nodeId,
                             const QString& propertyName,
                             const QVariant& oldValue,
                             const QVariant& newValue);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    bool canMergeWith(const Command* other) const override;
    bool mergeWith(const Command* other) override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QtNodes::NodeId m_nodeId;
    QString m_propertyName;
    QVariant m_oldValue;
    QVariant m_newValue;
};

/**
 * @brief 复制节点命令
 */
class CopyNodesCommand : public Command
{
public:
    CopyNodesCommand(QtNodes::DataFlowGraphicsScene* scene,
                    const QList<QtNodes::NodeId>& nodeIds,
                    const QPointF& offset);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;
    
    // 获取复制的节点ID列表（执行后可用）
    QList<QtNodes::NodeId> getCopiedNodeIds() const { return m_copiedNodeIds; }

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QList<QtNodes::NodeId> m_originalNodeIds;
    QList<QtNodes::NodeId> m_copiedNodeIds;
    QPointF m_offset;
    QJsonObject m_nodesData;
};

/**
 * @brief 粘贴节点命令
 */
class PasteNodesCommand : public Command
{
public:
    PasteNodesCommand(QtNodes::DataFlowGraphicsScene* scene,
                     const QJsonObject& nodesData,
                     const QPointF& position);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QJsonObject m_nodesData;
    QPointF m_position;
    QList<QtNodes::NodeId> m_pastedNodeIds;
};

/**
 * @brief 选择节点命令
 */
class SelectNodesCommand : public Command
{
public:
    SelectNodesCommand(QtNodes::DataFlowGraphicsScene* scene,
                      const QList<QtNodes::NodeId>& nodeIds,
                      bool addToSelection = false);
    
    bool execute() override;
    bool undo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QtNodes::DataFlowGraphicsScene* m_scene;
    QList<QtNodes::NodeId> m_newSelection;
    QList<QtNodes::NodeId> m_oldSelection;
    bool m_addToSelection;
};

// 注意：这些命令类型需要特定的构造参数，不能使用自动注册宏
// 如果需要序列化支持，应该在具体使用时手动注册
