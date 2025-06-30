#include "NodeCommands.hpp"
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QDebug>

// CreateNodeCommand 实现
CreateNodeCommand::CreateNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                                   const QString& nodeType,
                                   const QPointF& position)
    : m_scene(scene)
    , m_nodeType(nodeType)
    , m_position(position)
    , m_nodeId(QtNodes::NodeId{})
{
}

bool CreateNodeCommand::execute()
{
    if (!m_scene) {
        qWarning() << "CreateNodeCommand: Scene is null";
        return false;
    }

    try {
        // 通过场景的图模型创建节点
        auto& graphModel = m_scene->graphModel();
        m_nodeId = graphModel.addNode(m_nodeType);

        if (m_nodeId == QtNodes::NodeId{}) {
            qWarning() << "CreateNodeCommand: Failed to create node of type:" << m_nodeType;
            return false;
        }

        // 设置节点位置
        graphModel.setNodeData(m_nodeId, QtNodes::NodeRole::Position, m_position);

        qDebug() << "CreateNodeCommand: Created node" << m_nodeId << "of type" << m_nodeType;
        return true;

    } catch (const std::exception& e) {
        qWarning() << "CreateNodeCommand: Exception during execution:" << e.what();
        return false;
    }
}

bool CreateNodeCommand::undo()
{
    if (!m_scene) {
        qWarning() << "CreateNodeCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        if (!graphModel.nodeExists(m_nodeId)) {
            qWarning() << "CreateNodeCommand: Node not found for undo:" << m_nodeId;
            return false;
        }

        // 保存节点数据（如果需要的话）
        // m_nodeData = graphModel.nodeData(m_nodeId, QtNodes::NodeRole::Widget);

        // 删除节点
        graphModel.deleteNode(m_nodeId);

        qDebug() << "CreateNodeCommand: Removed node" << m_nodeId;
        return true;

    } catch (const std::exception& e) {
        qWarning() << "CreateNodeCommand: Exception during undo:" << e.what();
        return false;
    }
}

QString CreateNodeCommand::getDescription() const
{
    return QString("创建节点 (%1)").arg(m_nodeType);
}

QString CreateNodeCommand::getType() const
{
    return "CreateNodeCommand";
}

QJsonObject CreateNodeCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["nodeType"] = m_nodeType;
    json["position"] = QJsonObject{
        {"x", m_position.x()},
        {"y", m_position.y()}
    };
    // NodeId序列化需要根据实际实现调整
    // json["nodeId"] = m_nodeId.toString();
    if (!m_nodeData.isEmpty()) {
        json["nodeData"] = m_nodeData;
    }
    return json;
}

bool CreateNodeCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }
    
    m_nodeType = json["nodeType"].toString();
    
    auto posObj = json["position"].toObject();
    m_position = QPointF(posObj["x"].toDouble(), posObj["y"].toDouble());
    
    // NodeId的序列化需要根据实际的QtNodes版本调整
    // m_nodeId = QtNodes::NodeId::fromString(json["nodeId"].toString());
    
    if (json.contains("nodeData")) {
        m_nodeData = json["nodeData"].toObject();
    }
    
    return true;
}

// DeleteNodeCommand 实现
DeleteNodeCommand::DeleteNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                                   QtNodes::NodeId nodeId)
    : m_scene(scene)
    , m_nodeId(nodeId)
{
}

bool DeleteNodeCommand::execute()
{
    if (!m_scene) {
        qWarning() << "DeleteNodeCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        if (!graphModel.nodeExists(m_nodeId)) {
            qWarning() << "DeleteNodeCommand: Node not found:" << m_nodeId;
            return false;
        }

        // 保存节点信息
        m_position = graphModel.nodeData(m_nodeId, QtNodes::NodeRole::Position).value<QPointF>();
        // m_nodeType = graphModel.nodeData(m_nodeId, QtNodes::NodeRole::Type).toString();

        // 删除节点
        graphModel.deleteNode(m_nodeId);

        qDebug() << "DeleteNodeCommand: Deleted node" << m_nodeId;
        return true;

    } catch (const std::exception& e) {
        qWarning() << "DeleteNodeCommand: Exception during execution:" << e.what();
        return false;
    }
}

bool DeleteNodeCommand::undo()
{
    if (!m_scene) {
        qWarning() << "DeleteNodeCommand: Scene is null";
        return false;
    }

    try {
        // 简化的撤销实现 - 重新创建节点
        auto& graphModel = m_scene->graphModel();

        // 重新创建节点（这里简化处理，实际应该保存更多信息）
        QtNodes::NodeId newNodeId = graphModel.addNode(m_nodeType);
        if (newNodeId != QtNodes::NodeId{}) {
            // 恢复位置
            graphModel.setNodeData(newNodeId, QtNodes::NodeRole::Position, m_position);
            qDebug() << "DeleteNodeCommand: Restored node" << newNodeId;
            return true;
        }

        qWarning() << "DeleteNodeCommand: Failed to restore node";
        return false;

    } catch (const std::exception& e) {
        qWarning() << "DeleteNodeCommand: Exception during undo:" << e.what();
        return false;
    }
}

QString DeleteNodeCommand::getDescription() const
{
    return QString("删除节点 (%1)").arg(m_nodeType);
}

QString DeleteNodeCommand::getType() const
{
    return "DeleteNodeCommand";
}

QJsonObject DeleteNodeCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    // 简化的序列化
    json["nodeType"] = m_nodeType;
    json["position"] = QJsonObject{
        {"x", m_position.x()},
        {"y", m_position.y()}
    };

    return json;
}

bool DeleteNodeCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    m_nodeType = json["nodeType"].toString();

    auto posObj = json["position"].toObject();
    m_position = QPointF(posObj["x"].toDouble(), posObj["y"].toDouble());

    return true;
}

// MoveNodeCommand 实现
MoveNodeCommand::MoveNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                               QtNodes::NodeId nodeId,
                               const QPointF& oldPosition,
                               const QPointF& newPosition)
    : m_scene(scene)
    , m_nodeId(nodeId)
    , m_oldPosition(oldPosition)
    , m_newPosition(newPosition)
{
}

bool MoveNodeCommand::execute()
{
    if (!m_scene) {
        qWarning() << "MoveNodeCommand: Scene is null";
        return false;
    }

    auto& graphModel = m_scene->graphModel();
    if (!graphModel.nodeExists(m_nodeId)) {
        qWarning() << "MoveNodeCommand: Node not found:" << m_nodeId;
        return false;
    }

    graphModel.setNodeData(m_nodeId, QtNodes::NodeRole::Position, m_newPosition);
    qDebug() << "MoveNodeCommand: Moved node" << m_nodeId << "to" << m_newPosition;
    return true;
}

bool MoveNodeCommand::undo()
{
    if (!m_scene) {
        qWarning() << "MoveNodeCommand: Scene is null";
        return false;
    }

    auto& graphModel = m_scene->graphModel();
    if (!graphModel.nodeExists(m_nodeId)) {
        qWarning() << "MoveNodeCommand: Node not found:" << m_nodeId;
        return false;
    }

    graphModel.setNodeData(m_nodeId, QtNodes::NodeRole::Position, m_oldPosition);
    qDebug() << "MoveNodeCommand: Moved node" << m_nodeId << "back to" << m_oldPosition;
    return true;
}

QString MoveNodeCommand::getDescription() const
{
    return QString("移动节点");
}

QString MoveNodeCommand::getType() const
{
    return "MoveNodeCommand";
}

bool MoveNodeCommand::canMergeWith(const Command* other) const
{
    auto otherMove = dynamic_cast<const MoveNodeCommand*>(other);
    if (!otherMove) {
        return false;
    }
    
    // 只有同一个节点的移动命令才能合并
    return m_nodeId == otherMove->m_nodeId;
}

bool MoveNodeCommand::mergeWith(const Command* other)
{
    auto otherMove = dynamic_cast<const MoveNodeCommand*>(other);
    if (!otherMove || !canMergeWith(other)) {
        return false;
    }
    
    // 更新最终位置
    m_newPosition = otherMove->m_newPosition;
    qDebug() << "MoveNodeCommand: Merged move commands for node" << m_nodeId;
    return true;
}

QJsonObject MoveNodeCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    // 简化的序列化
    json["oldPosition"] = QJsonObject{
        {"x", m_oldPosition.x()},
        {"y", m_oldPosition.y()}
    };
    json["newPosition"] = QJsonObject{
        {"x", m_newPosition.x()},
        {"y", m_newPosition.y()}
    };
    return json;
}

bool MoveNodeCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    auto oldPosObj = json["oldPosition"].toObject();
    m_oldPosition = QPointF(oldPosObj["x"].toDouble(), oldPosObj["y"].toDouble());

    auto newPosObj = json["newPosition"].toObject();
    m_newPosition = QPointF(newPosObj["x"].toDouble(), newPosObj["y"].toDouble());

    return true;
}
