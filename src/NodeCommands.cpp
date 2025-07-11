#include "NodeCommands.hpp"
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QDebug>
#include <QClipboard>
#include <QApplication>

// CreateNodeCommand 实现
CreateNodeCommand::CreateNodeCommand(QtNodes::DataFlowGraphicsScene* scene,
                                   const QString& nodeType,
                                   const QPointF& position)
    : m_scene(scene)
    , m_nodeType(nodeType)
    , m_position(position)
    , m_nodeId(QtNodes::InvalidNodeId)
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

        if (m_nodeId == QtNodes::InvalidNodeId) {
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

        // 保存节点的所有数据用于重做
        m_nodeData = graphModel.saveNode(m_nodeId);

        // 保存所有相关连接的详细信息
        m_connectionDetails.clear();
        
        // 获取该节点的所有连接 - 转换为正确的类型
        std::unordered_set<QtNodes::ConnectionId> allConnections;
        auto* dataFlowModel = dynamic_cast<QtNodes::DataFlowGraphModel*>(&graphModel);
        if (dataFlowModel) {
            auto nodeDelegate = dataFlowModel->delegateModel<QtNodes::NodeDelegateModel>(m_nodeId);
            if (nodeDelegate) {
                // 检查所有输出端口的连接
                unsigned int outputPorts = nodeDelegate->nPorts(QtNodes::PortType::Out);
                for (unsigned int portIndex = 0; portIndex < outputPorts; ++portIndex) {
                    auto connections = graphModel.connections(m_nodeId, QtNodes::PortType::Out, portIndex);
                    for (const auto& conn : connections) {
                        allConnections.insert(conn);
                    }
                }
                
                // 检查所有输入端口的连接
                unsigned int inputPorts = nodeDelegate->nPorts(QtNodes::PortType::In);
                for (unsigned int portIndex = 0; portIndex < inputPorts; ++portIndex) {
                    auto connections = graphModel.connections(m_nodeId, QtNodes::PortType::In, portIndex);
                    for (const auto& conn : connections) {
                        allConnections.insert(conn);
                    }
                }
            }
        }
        
        for (const auto& connectionId : allConnections) {
            // 保存连接的详细信息
            ConnectionDetail detail;
            detail.connectionId = connectionId;
            detail.outputNodeId = connectionId.outNodeId;
            detail.outputPortIndex = connectionId.outPortIndex;
            detail.inputNodeId = connectionId.inNodeId;
            detail.inputPortIndex = connectionId.inPortIndex;
            
            // 如果这个节点是输出节点，需要记录输入节点的信息
            if (connectionId.outNodeId == m_nodeId) {
                detail.isOutput = true;
                detail.otherNodeId = connectionId.inNodeId;
                detail.otherPortIndex = connectionId.inPortIndex;
            } else {
                // 这个节点是输入节点
                detail.isOutput = false;
                detail.otherNodeId = connectionId.outNodeId;
                detail.otherPortIndex = connectionId.outPortIndex;
            }
            
            m_connectionDetails.append(detail);
        }

        // 删除节点
        graphModel.deleteNode(m_nodeId);

        qDebug() << "CreateNodeCommand: Removed node" << m_nodeId << "with" << m_connectionDetails.size() << "connections";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "CreateNodeCommand: Exception during undo:" << e.what();
        return false;
    }
}

bool CreateNodeCommand::redo()
{
    if (!m_scene) {
        qWarning() << "CreateNodeCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        if (m_nodeData.isEmpty()) {
            qWarning() << "CreateNodeCommand: No saved node data for redo";
            return false;
        }

        // 使用保存的数据恢复节点
        graphModel.loadNode(m_nodeData);

        // 验证节点是否成功恢复
        if (!graphModel.nodeExists(m_nodeId)) {
            qWarning() << "CreateNodeCommand: Failed to restore node" << m_nodeId;
            return false;
        }

        // 恢复所有连接
        for (const auto& detail : m_connectionDetails) {
            // 验证两个节点都存在
            if (!graphModel.nodeExists(detail.outputNodeId) || 
                !graphModel.nodeExists(detail.inputNodeId)) {
                qWarning() << "CreateNodeCommand: Cannot restore connection - nodes don't exist:"
                          << detail.outputNodeId << "->" << detail.inputNodeId;
                continue;
            }

            try {
                // 创建连接
                QtNodes::ConnectionId connectionId{
                    detail.outputNodeId, detail.outputPortIndex,
                    detail.inputNodeId, detail.inputPortIndex
                };
                graphModel.addConnection(connectionId);

                qDebug() << "CreateNodeCommand: Restored connection:" 
                         << detail.outputNodeId << ":" << detail.outputPortIndex 
                         << "->" << detail.inputNodeId << ":" << detail.inputPortIndex;

            } catch (const std::exception& e) {
                qWarning() << "CreateNodeCommand: Failed to restore connection:" << e.what();
            }
        }

        qDebug() << "CreateNodeCommand: Restored node" << m_nodeId << "with" << m_connectionDetails.size() << "connections";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "CreateNodeCommand: Exception during redo:" << e.what();
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

        // 保存节点信息用于撤销
        m_position = graphModel.nodeData(m_nodeId, QtNodes::NodeRole::Position).value<QPointF>();
        
        // 保存完整的节点数据
        m_nodeData = graphModel.saveNode(m_nodeId);
        
        // 获取节点类型（从保存的数据中获取）
        if (m_nodeData.contains("type")) {
            m_nodeType = m_nodeData["type"].toString();
        }

        // 保存所有相关连接的详细信息
        m_connections.clear();
        m_connectionDetails.clear();
        
        // 获取该节点的所有连接 - 转换为正确的类型
        std::unordered_set<QtNodes::ConnectionId> allConnections;
        auto* dataFlowModel = dynamic_cast<QtNodes::DataFlowGraphModel*>(&graphModel);
        if (dataFlowModel) {
            auto nodeDelegate = dataFlowModel->delegateModel<QtNodes::NodeDelegateModel>(m_nodeId);
            if (nodeDelegate) {
                // 检查所有输出端口的连接
                unsigned int outputPorts = nodeDelegate->nPorts(QtNodes::PortType::Out);
                for (unsigned int portIndex = 0; portIndex < outputPorts; ++portIndex) {
                    auto connections = graphModel.connections(m_nodeId, QtNodes::PortType::Out, portIndex);
                    for (const auto& conn : connections) {
                        allConnections.insert(conn);
                    }
                }
                
                // 检查所有输入端口的连接
                unsigned int inputPorts = nodeDelegate->nPorts(QtNodes::PortType::In);
                for (unsigned int portIndex = 0; portIndex < inputPorts; ++portIndex) {
                    auto connections = graphModel.connections(m_nodeId, QtNodes::PortType::In, portIndex);
                    for (const auto& conn : connections) {
                        allConnections.insert(conn);
                    }
                }
            }
        }
        
        for (const auto& connectionId : allConnections) {
            m_connections.append(connectionId);
            
            // 保存连接的详细信息
            ConnectionDetail detail;
            detail.connectionId = connectionId;
            detail.outputNodeId = connectionId.outNodeId;
            detail.outputPortIndex = connectionId.outPortIndex;
            detail.inputNodeId = connectionId.inNodeId;
            detail.inputPortIndex = connectionId.inPortIndex;
            
            // 如果这个节点是输出节点，需要记录输入节点的信息
            if (connectionId.outNodeId == m_nodeId) {
                detail.isOutput = true;
                detail.otherNodeId = connectionId.inNodeId;
                detail.otherPortIndex = connectionId.inPortIndex;
            } else {
                // 这个节点是输入节点
                detail.isOutput = false;
                detail.otherNodeId = connectionId.outNodeId;
                detail.otherPortIndex = connectionId.outPortIndex;
            }
            
            m_connectionDetails.append(detail);
        }

        // 删除节点（这会自动删除相关连接）
        graphModel.deleteNode(m_nodeId);

        qDebug() << "DeleteNodeCommand: Deleted node" << m_nodeId << "with" << m_connections.size() << "connections";
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
        auto& graphModel = m_scene->graphModel();

        // 如果有完整的节点数据，恢复它
        if (!m_nodeData.isEmpty()) {
            // loadNode可能会改变节点ID，我们需要找到新创建的节点
            auto oldNodeIds = graphModel.allNodeIds();
            
            graphModel.loadNode(m_nodeData);
            
            // 找到新创建的节点ID
            auto newNodeIds = graphModel.allNodeIds();
            for (const auto& nodeId : newNodeIds) {
                if (oldNodeIds.find(nodeId) == oldNodeIds.end()) {
                    // 这是新创建的节点
                    m_nodeId = nodeId;
                    break;
                }
            }
            
            qDebug() << "DeleteNodeCommand: Restored node with ID" << m_nodeId;
        } else {
            // 如果没有保存的数据，重新创建（简化方法）
        QtNodes::NodeId newNodeId = graphModel.addNode(m_nodeType);
            if (newNodeId == QtNodes::NodeId{}) {
                qWarning() << "DeleteNodeCommand: Failed to restore node";
                return false;
            }

            // 恢复位置
            graphModel.setNodeData(newNodeId, QtNodes::NodeRole::Position, m_position);
            
            // 更新节点ID
            m_nodeId = newNodeId;

            qDebug() << "DeleteNodeCommand: Restored node" << newNodeId << "(fallback method)";
        }

        // 重新创建所有相关连接
        int restoredConnections = 0;
        for (const auto& detail : m_connectionDetails) {
            try {
                QtNodes::ConnectionId newConnectionId;
                
                if (detail.isOutput) {
                    // 删除的节点是输出节点
                    newConnectionId = {m_nodeId, detail.outputPortIndex, detail.otherNodeId, detail.inputPortIndex};
                } else {
                    // 删除的节点是输入节点
                    newConnectionId = {detail.otherNodeId, detail.outputPortIndex, m_nodeId, detail.inputPortIndex};
                }
                
                // 检查目标节点是否还存在
                if (!graphModel.nodeExists(detail.otherNodeId)) {
                    qWarning() << "DeleteNodeCommand: Target node" << detail.otherNodeId << "no longer exists, skipping connection";
                    continue;
                }
                
                // 检查连接是否可能
                if (graphModel.connectionPossible(newConnectionId)) {
                    graphModel.addConnection(newConnectionId);
                    restoredConnections++;
                    qDebug() << "DeleteNodeCommand: Restored connection between nodes" << newConnectionId.outNodeId << "and" << newConnectionId.inNodeId;
                } else {
                    qWarning() << "DeleteNodeCommand: Connection not possible, skipping";
                }
                
            } catch (const std::exception& e) {
                qWarning() << "DeleteNodeCommand: Failed to restore connection:" << e.what();
            }
        }
        
        qDebug() << "DeleteNodeCommand: Restored" << restoredConnections << "out of" << m_connectionDetails.size() << "connections";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "DeleteNodeCommand: Exception during undo:" << e.what();
        return false;
    }
}

bool DeleteNodeCommand::redo()
{
    if (!m_scene) {
        qWarning() << "DeleteNodeCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        // 检查当前节点是否存在
        if (!graphModel.nodeExists(m_nodeId)) {
            qWarning() << "DeleteNodeCommand: Node not found for redo:" << m_nodeId;
            return false;
        }

        // 直接删除节点（不需要重新保存数据，因为已经保存过了）
        graphModel.deleteNode(m_nodeId);

        qDebug() << "DeleteNodeCommand: Re-deleted node" << m_nodeId;
        return true;

    } catch (const std::exception& e) {
        qWarning() << "DeleteNodeCommand: Exception during redo:" << e.what();
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

// CreateConnectionCommand 实现
CreateConnectionCommand::CreateConnectionCommand(QtNodes::DataFlowGraphicsScene* scene,
                                               QtNodes::NodeId outputNodeId,
                                               QtNodes::PortIndex outputPortIndex,
                                               QtNodes::NodeId inputNodeId,
                                               QtNodes::PortIndex inputPortIndex)
    : m_scene(scene)
    , m_outputNodeId(outputNodeId)
    , m_outputPortIndex(outputPortIndex)
    , m_inputNodeId(inputNodeId)
    , m_inputPortIndex(inputPortIndex)
    , m_connectionId{}
{
}

bool CreateConnectionCommand::execute()
{
    if (!m_scene) {
        qWarning() << "CreateConnectionCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        // 检查节点是否存在
        if (!graphModel.nodeExists(m_outputNodeId) || !graphModel.nodeExists(m_inputNodeId)) {
            qWarning() << "CreateConnectionCommand: One or both nodes not found";
            return false;
        }

        // 创建连接
        m_connectionId = {m_outputNodeId, m_outputPortIndex, m_inputNodeId, m_inputPortIndex};
        
        // 检查连接是否可能
        if (!graphModel.connectionPossible(m_connectionId)) {
            qWarning() << "CreateConnectionCommand: Connection not possible";
            return false;
        }
        
        // 创建连接
        graphModel.addConnection(m_connectionId);
        
        qDebug() << "CreateConnectionCommand: Created connection successfully";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "CreateConnectionCommand: Exception during execution:" << e.what();
        return false;
    }
}

bool CreateConnectionCommand::undo()
{
    if (!m_scene) {
        qWarning() << "CreateConnectionCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        // 检查连接是否存在
        if (!graphModel.connectionExists(m_connectionId)) {
            qWarning() << "CreateConnectionCommand: Connection doesn't exist for undo";
            return false;
        }

        // 删除连接
        graphModel.deleteConnection(m_connectionId);

        qDebug() << "CreateConnectionCommand: Removed connection successfully";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "CreateConnectionCommand: Exception during undo:" << e.what();
        return false;
    }
}

QString CreateConnectionCommand::getDescription() const
{
    return QString("创建连接");
}

QString CreateConnectionCommand::getType() const
{
    return "CreateConnectionCommand";
}

QJsonObject CreateConnectionCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["outputNodeId"] = QString::number(m_outputNodeId);
    json["outputPortIndex"] = static_cast<int>(m_outputPortIndex);
    json["inputNodeId"] = QString::number(m_inputNodeId);
    json["inputPortIndex"] = static_cast<int>(m_inputPortIndex);
    return json;
}

bool CreateConnectionCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    m_outputNodeId = json["outputNodeId"].toString().toULongLong();
    m_outputPortIndex = json["outputPortIndex"].toInt();
    m_inputNodeId = json["inputNodeId"].toString().toULongLong();
    m_inputPortIndex = json["inputPortIndex"].toInt();

    return true;
}

// DeleteConnectionCommand 实现
DeleteConnectionCommand::DeleteConnectionCommand(QtNodes::DataFlowGraphicsScene* scene,
                                               QtNodes::ConnectionId connectionId)
    : m_scene(scene)
    , m_connectionId(connectionId)
    , m_outputNodeId(connectionId.outNodeId)
    , m_outputPortIndex(connectionId.outPortIndex)
    , m_inputNodeId(connectionId.inNodeId)
    , m_inputPortIndex(connectionId.inPortIndex)
{
}

bool DeleteConnectionCommand::execute()
{
    if (!m_scene) {
        qWarning() << "DeleteConnectionCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        // 删除连接
        graphModel.deleteConnection(m_connectionId);

        qDebug() << "DeleteConnectionCommand: Deleted connection successfully";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "DeleteConnectionCommand: Exception during execution:" << e.what();
        return false;
    }
}

bool DeleteConnectionCommand::undo()
{
    if (!m_scene) {
        qWarning() << "DeleteConnectionCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        // 重新创建连接
        QtNodes::ConnectionId restoreConnectionId = {m_outputNodeId, m_outputPortIndex, m_inputNodeId, m_inputPortIndex};
        
        // 检查连接是否可能
        if (!graphModel.connectionPossible(restoreConnectionId)) {
            qWarning() << "DeleteConnectionCommand: Cannot restore connection - not possible";
            return false;
        }
        
        // 创建连接
        graphModel.addConnection(restoreConnectionId);
        
        qDebug() << "DeleteConnectionCommand: Restored connection successfully";
        return true;

    } catch (const std::exception& e) {
        qWarning() << "DeleteConnectionCommand: Exception during undo:" << e.what();
        return false;
    }
}

QString DeleteConnectionCommand::getDescription() const
{
    return QString("删除连接");
}

QString DeleteConnectionCommand::getType() const
{
    return "DeleteConnectionCommand";
}

QJsonObject DeleteConnectionCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["outputNodeId"] = QString::number(m_outputNodeId);
    json["outputPortIndex"] = static_cast<int>(m_outputPortIndex);
    json["inputNodeId"] = QString::number(m_inputNodeId);
    json["inputPortIndex"] = static_cast<int>(m_inputPortIndex);
    return json;
}

bool DeleteConnectionCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    m_outputNodeId = json["outputNodeId"].toString().toULongLong();
    m_outputPortIndex = json["outputPortIndex"].toInt();
    m_inputNodeId = json["inputNodeId"].toString().toULongLong();
    m_inputPortIndex = json["inputPortIndex"].toInt();

    return true;
}

// ModifyNodePropertyCommand 实现
ModifyNodePropertyCommand::ModifyNodePropertyCommand(QtNodes::DataFlowGraphicsScene* scene,
                                                   QtNodes::NodeId nodeId,
                                                   const QString& propertyName,
                                                   const QVariant& oldValue,
                                                   const QVariant& newValue)
    : m_scene(scene)
    , m_nodeId(nodeId)
    , m_propertyName(propertyName)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
{
}

bool ModifyNodePropertyCommand::execute()
{
    if (!m_scene) {
        qWarning() << "ModifyNodePropertyCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        if (!graphModel.nodeExists(m_nodeId)) {
            qWarning() << "ModifyNodePropertyCommand: Node not found:" << m_nodeId;
            return false;
        }

        // 属性修改的简化实现
        // 实际应用中需要根据具体的属性系统来实现
        qDebug() << "ModifyNodePropertyCommand: Property modification not fully implemented";
        qDebug() << "  Property:" << m_propertyName << "Value:" << m_newValue;
        return true;

    } catch (const std::exception& e) {
        qWarning() << "ModifyNodePropertyCommand: Exception during execution:" << e.what();
        return false;
    }
}

bool ModifyNodePropertyCommand::undo()
{
    if (!m_scene) {
        qWarning() << "ModifyNodePropertyCommand: Scene is null";
        return false;
    }

    try {
        auto& graphModel = m_scene->graphModel();

        if (!graphModel.nodeExists(m_nodeId)) {
            qWarning() << "ModifyNodePropertyCommand: Node not found:" << m_nodeId;
            return false;
        }

        // 属性恢复的简化实现
        qDebug() << "ModifyNodePropertyCommand: Property restore not fully implemented";
        qDebug() << "  Property:" << m_propertyName << "Restored to:" << m_oldValue;
        return true;

    } catch (const std::exception& e) {
        qWarning() << "ModifyNodePropertyCommand: Exception during undo:" << e.what();
        return false;
    }
}

QString ModifyNodePropertyCommand::getDescription() const
{
    return QString("修改属性 %1").arg(m_propertyName);
}

QString ModifyNodePropertyCommand::getType() const
{
    return "ModifyNodePropertyCommand";
}

bool ModifyNodePropertyCommand::canMergeWith(const Command* other) const
{
    auto otherModify = dynamic_cast<const ModifyNodePropertyCommand*>(other);
    if (!otherModify) {
        return false;
    }
    
    // 只有同一个节点的同一个属性的修改命令才能合并
    return m_nodeId == otherModify->m_nodeId && m_propertyName == otherModify->m_propertyName;
}

bool ModifyNodePropertyCommand::mergeWith(const Command* other)
{
    auto otherModify = dynamic_cast<const ModifyNodePropertyCommand*>(other);
    if (!otherModify || !canMergeWith(other)) {
        return false;
    }
    
    // 更新最终值
    m_newValue = otherModify->m_newValue;
    qDebug() << "ModifyNodePropertyCommand: Merged property commands for" << m_propertyName;
    return true;
}

QJsonObject ModifyNodePropertyCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["nodeId"] = QString::number(m_nodeId);
    json["propertyName"] = m_propertyName;
    json["oldValue"] = QJsonValue::fromVariant(m_oldValue);
    json["newValue"] = QJsonValue::fromVariant(m_newValue);
    return json;
}

bool ModifyNodePropertyCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    m_nodeId = json["nodeId"].toString().toULongLong();
    m_propertyName = json["propertyName"].toString();
    m_oldValue = json["oldValue"].toVariant();
    m_newValue = json["newValue"].toVariant();

    return true;
}

// CopyNodesCommand 实现（暂时简化）
CopyNodesCommand::CopyNodesCommand(QtNodes::DataFlowGraphicsScene* scene,
                                 const QList<QtNodes::NodeId>& nodeIds,
                                 const QPointF& offset)
    : m_scene(scene)
    , m_originalNodeIds(nodeIds)
    , m_offset(offset)
{
}

bool CopyNodesCommand::execute()
{
    // 这里简化实现，实际应该实现真正的复制逻辑
    qDebug() << "CopyNodesCommand: Execute (simplified implementation)";
    return true;
}

bool CopyNodesCommand::undo()
{
    // 删除复制的节点
    if (m_scene) {
        auto& graphModel = m_scene->graphModel();
        for (const auto& nodeId : m_copiedNodeIds) {
            if (graphModel.nodeExists(nodeId)) {
                graphModel.deleteNode(nodeId);
            }
        }
        m_copiedNodeIds.clear();
    }
    
    qDebug() << "CopyNodesCommand: Undo";
    return true;
}

QString CopyNodesCommand::getDescription() const
{
    return QString("复制 %1 个节点").arg(m_originalNodeIds.size());
}

QString CopyNodesCommand::getType() const
{
    return "CopyNodesCommand";
}

QJsonObject CopyNodesCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    // 简化的序列化
    return json;
}

bool CopyNodesCommand::fromJson(const QJsonObject& json)
{
    return Command::fromJson(json);
}

// PasteNodesCommand 实现（暂时简化）
PasteNodesCommand::PasteNodesCommand(QtNodes::DataFlowGraphicsScene* scene,
                                   const QJsonObject& nodesData,
                                   const QPointF& position)
    : m_scene(scene)
    , m_nodesData(nodesData)
    , m_position(position)
{
}

bool PasteNodesCommand::execute()
{
    qDebug() << "PasteNodesCommand: Execute (simplified implementation)";
    return true;
}

bool PasteNodesCommand::undo()
{
    // 删除粘贴的节点
    if (m_scene) {
        auto& graphModel = m_scene->graphModel();
        for (const auto& nodeId : m_pastedNodeIds) {
            if (graphModel.nodeExists(nodeId)) {
                graphModel.deleteNode(nodeId);
            }
        }
        m_pastedNodeIds.clear();
    }
    
    qDebug() << "PasteNodesCommand: Undo";
    return true;
}

QString PasteNodesCommand::getDescription() const
{
    return QString("粘贴节点");
}

QString PasteNodesCommand::getType() const
{
    return "PasteNodesCommand";
}

QJsonObject PasteNodesCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["nodesData"] = m_nodesData;
    json["position"] = QJsonObject{
        {"x", m_position.x()},
        {"y", m_position.y()}
    };
    return json;
}

bool PasteNodesCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    m_nodesData = json["nodesData"].toObject();
    
    auto posObj = json["position"].toObject();
    m_position = QPointF(posObj["x"].toDouble(), posObj["y"].toDouble());

    return true;
}

// SelectNodesCommand 实现
SelectNodesCommand::SelectNodesCommand(QtNodes::DataFlowGraphicsScene* scene,
                                     const QList<QtNodes::NodeId>& nodeIds,
                                     bool addToSelection)
    : m_scene(scene)
    , m_newSelection(nodeIds)
    , m_addToSelection(addToSelection)
{
}

bool SelectNodesCommand::execute()
{
    if (!m_scene) {
        qWarning() << "SelectNodesCommand: Scene is null";
        return false;
    }

    // 保存当前选择状态（简化实现）
    // 实际应该通过场景获取当前选择的节点
    
    // 设置新的选择（这里需要根据QtNodes的API实现）
    // 简化实现
    qDebug() << "SelectNodesCommand: Selected" << m_newSelection.size() << "nodes";
    return true;
}

bool SelectNodesCommand::undo()
{
    if (!m_scene) {
        qWarning() << "SelectNodesCommand: Scene is null";
        return false;
    }

    // 恢复旧的选择状态
    qDebug() << "SelectNodesCommand: Restored previous selection";
    return true;
}

QString SelectNodesCommand::getDescription() const
{
    return QString("选择 %1 个节点").arg(m_newSelection.size());
}

QString SelectNodesCommand::getType() const
{
    return "SelectNodesCommand";
}

QJsonObject SelectNodesCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["addToSelection"] = m_addToSelection;
    
    QJsonArray newSelectionArray;
    for (const auto& nodeId : m_newSelection) {
        newSelectionArray.append(QString::number(nodeId));
    }
    json["newSelection"] = newSelectionArray;
    
    QJsonArray oldSelectionArray;
    for (const auto& nodeId : m_oldSelection) {
        oldSelectionArray.append(QString::number(nodeId));
    }
    json["oldSelection"] = oldSelectionArray;
    
    return json;
}

bool SelectNodesCommand::fromJson(const QJsonObject& json)
{
    if (!Command::fromJson(json)) {
        return false;
    }

    m_addToSelection = json["addToSelection"].toBool();
    
    m_newSelection.clear();
    auto newSelectionArray = json["newSelection"].toArray();
    for (const auto& value : newSelectionArray) {
        m_newSelection.append(value.toString().toULongLong());
    }
    
    m_oldSelection.clear();
    auto oldSelectionArray = json["oldSelection"].toArray();
    for (const auto& value : oldSelectionArray) {
        m_oldSelection.append(value.toString().toULongLong());
    }

    return true;
}
