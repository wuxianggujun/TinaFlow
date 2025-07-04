#include "Block.hpp"
#include <QUuid>
#include <QDebug>

Block::Block(const QString& id, BlockType type, const QString& title)
    : m_id(id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id)
    , m_type(type)
    , m_title(title)
    , m_bounds(0, 0, 120, 40)  // 默认大小
{
    // 设置默认颜色
    m_color = BlockFactory::getBlockTypeColor(type);
}

void Block::addConnectionPoint(const ConnectionPoint& point)
{
    // 检查是否已存在相同ID的连接点
    for (auto& existingPoint : m_connectionPoints) {
        if (existingPoint.id == point.id) {
            existingPoint = point;  // 更新现有连接点
            return;
        }
    }
    m_connectionPoints.append(point);
}

void Block::removeConnectionPoint(const QString& pointId)
{
    for (int i = 0; i < m_connectionPoints.size(); ++i) {
        if (m_connectionPoints[i].id == pointId) {
            m_connectionPoints.removeAt(i);
            break;
        }
    }
}

ConnectionPoint* Block::getConnectionPoint(const QString& pointId)
{
    for (auto& point : m_connectionPoints) {
        if (point.id == pointId) {
            return &point;
        }
    }
    return nullptr;
}

void Block::setParameter(const QString& key, const QVariant& value)
{
    if (value.canConvert<QString>()) {
        m_parameters[key] = value.toString();
    } else if (value.canConvert<double>()) {
        m_parameters[key] = value.toDouble();
    } else if (value.canConvert<bool>()) {
        m_parameters[key] = value.toBool();
    } else if (value.canConvert<QJsonArray>()) {
        m_parameters[key] = value.toJsonArray();
    } else if (value.canConvert<QJsonObject>()) {
        m_parameters[key] = value.toJsonObject();
    }
}

QVariant Block::getParameter(const QString& key) const
{
    if (m_parameters.contains(key)) {
        return QVariant::fromValue(m_parameters[key]);
    }
    return QVariant();
}

QJsonObject Block::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["type"] = static_cast<int>(m_type);
    json["title"] = m_title;
    json["description"] = m_description;
    json["position"] = QJsonObject{{"x", m_position.x()}, {"y", m_position.y()}};
    json["bounds"] = QJsonObject{{"x", m_bounds.x()}, {"y", m_bounds.y()}, 
                                {"width", m_bounds.width()}, {"height", m_bounds.height()}};
    json["color"] = QJsonObject{{"r", m_color.red()}, {"g", m_color.green()}, 
                               {"b", m_color.blue()}, {"a", m_color.alpha()}};
    json["selected"] = m_selected;
    json["enabled"] = m_enabled;
    json["parameters"] = m_parameters;
    
    // 序列化连接点
    QJsonArray connectionPointsArray;
    for (const auto& point : m_connectionPoints) {
        QJsonObject pointJson;
        pointJson["id"] = point.id;
        pointJson["name"] = point.name;
        pointJson["type"] = static_cast<int>(point.type);
        pointJson["dataType"] = static_cast<int>(point.dataType);
        pointJson["position"] = QJsonObject{{"x", point.position.x()}, {"y", point.position.y()}};
        pointJson["color"] = QJsonObject{{"r", point.color.red()}, {"g", point.color.green()}, 
                                        {"b", point.color.blue()}, {"a", point.color.alpha()}};
        pointJson["isConnected"] = point.isConnected;
        pointJson["connectedBlockId"] = point.connectedBlockId;
        pointJson["connectedPointId"] = point.connectedPointId;
        connectionPointsArray.append(pointJson);
    }
    json["connectionPoints"] = connectionPointsArray;
    
    return json;
}

void Block::fromJson(const QJsonObject& json)
{
    m_id = json["id"].toString();
    m_type = static_cast<BlockType>(json["type"].toInt());
    m_title = json["title"].toString();
    m_description = json["description"].toString();
    
    QJsonObject posJson = json["position"].toObject();
    m_position = QPointF(posJson["x"].toDouble(), posJson["y"].toDouble());
    
    QJsonObject boundsJson = json["bounds"].toObject();
    m_bounds = QRectF(boundsJson["x"].toDouble(), boundsJson["y"].toDouble(),
                     boundsJson["width"].toDouble(), boundsJson["height"].toDouble());
    
    QJsonObject colorJson = json["color"].toObject();
    m_color = QColor(colorJson["r"].toInt(), colorJson["g"].toInt(), 
                    colorJson["b"].toInt(), colorJson["a"].toInt());
    
    m_selected = json["selected"].toBool();
    m_enabled = json["enabled"].toBool();
    m_parameters = json["parameters"].toObject();
    
    // 反序列化连接点
    m_connectionPoints.clear();
    QJsonArray connectionPointsArray = json["connectionPoints"].toArray();
    for (const auto& pointValue : connectionPointsArray) {
        QJsonObject pointJson = pointValue.toObject();
        ConnectionPoint point;
        point.id = pointJson["id"].toString();
        point.name = pointJson["name"].toString();
        point.type = static_cast<ConnectionType>(pointJson["type"].toInt());
        point.dataType = static_cast<BlockDataType>(pointJson["dataType"].toInt());
        
        QJsonObject pointPosJson = pointJson["position"].toObject();
        point.position = QPointF(pointPosJson["x"].toDouble(), pointPosJson["y"].toDouble());
        
        QJsonObject pointColorJson = pointJson["color"].toObject();
        point.color = QColor(pointColorJson["r"].toInt(), pointColorJson["g"].toInt(), 
                           pointColorJson["b"].toInt(), pointColorJson["a"].toInt());
        
        point.isConnected = pointJson["isConnected"].toBool();
        point.connectedBlockId = pointJson["connectedBlockId"].toString();
        point.connectedPointId = pointJson["connectedPointId"].toString();
        
        m_connectionPoints.append(point);
    }
}

// BlockFactory 实现
std::shared_ptr<Block> BlockFactory::createBlock(BlockType type, const QString& id)
{
    QString title = getBlockTypeName(type);
    auto block = std::make_shared<Block>(id, type, title);
    
    // 设置积木颜色
    block->setColor(getBlockTypeColor(type));
    
    // 根据类型添加连接点
    switch (type) {
        case BlockType::Start:
            block->addConnectionPoint(ConnectionPoint("next", "下一步", ConnectionType::Next, 
                                                    BlockDataType::Flow, QPointF(120, 20), QColor(100, 100, 100)));
            break;
            
        case BlockType::End:
            block->addConnectionPoint(ConnectionPoint("prev", "上一步", ConnectionType::Previous, 
                                                    BlockDataType::Flow, QPointF(0, 20), QColor(100, 100, 100)));
            break;
            
        case BlockType::Variable:
            block->addConnectionPoint(ConnectionPoint("prev", "上一步", ConnectionType::Previous, 
                                                    BlockDataType::Flow, QPointF(0, 20), QColor(100, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("next", "下一步", ConnectionType::Next, 
                                                    BlockDataType::Flow, QPointF(120, 20), QColor(100, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("value", "值", ConnectionType::Output, 
                                                    BlockDataType::Any, QPointF(120, 10), QColor(255, 200, 0)));
            break;
            
        case BlockType::IfElse:
            block->addConnectionPoint(ConnectionPoint("prev", "上一步", ConnectionType::Previous, 
                                                    BlockDataType::Flow, QPointF(0, 20), QColor(100, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("condition", "条件", ConnectionType::Input, 
                                                    BlockDataType::Boolean, QPointF(0, 10), QColor(0, 255, 0)));
            block->addConnectionPoint(ConnectionPoint("true", "真", ConnectionType::Next, 
                                                    BlockDataType::Flow, QPointF(60, 40), QColor(0, 255, 0)));
            block->addConnectionPoint(ConnectionPoint("false", "假", ConnectionType::Next, 
                                                    BlockDataType::Flow, QPointF(120, 40), QColor(255, 0, 0)));
            break;
            
        case BlockType::Math:
            block->addConnectionPoint(ConnectionPoint("input1", "输入1", ConnectionType::Input, 
                                                    BlockDataType::Number, QPointF(0, 10), QColor(0, 0, 255)));
            block->addConnectionPoint(ConnectionPoint("input2", "输入2", ConnectionType::Input, 
                                                    BlockDataType::Number, QPointF(0, 30), QColor(0, 0, 255)));
            block->addConnectionPoint(ConnectionPoint("result", "结果", ConnectionType::Output, 
                                                    BlockDataType::Number, QPointF(120, 20), QColor(0, 0, 255)));
            break;
            
        case BlockType::GetCell:
            block->addConnectionPoint(ConnectionPoint("prev", "上一步", ConnectionType::Previous, 
                                                    BlockDataType::Flow, QPointF(0, 20), QColor(100, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("next", "下一步", ConnectionType::Next, 
                                                    BlockDataType::Flow, QPointF(120, 20), QColor(100, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("address", "地址", ConnectionType::Input, 
                                                    BlockDataType::String, QPointF(0, 10), QColor(255, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("value", "值", ConnectionType::Output, 
                                                    BlockDataType::Any, QPointF(120, 10), QColor(255, 100, 100)));
            break;
            
        default:
            // 默认连接点
            block->addConnectionPoint(ConnectionPoint("prev", "上一步", ConnectionType::Previous, 
                                                    BlockDataType::Flow, QPointF(0, 20), QColor(100, 100, 100)));
            block->addConnectionPoint(ConnectionPoint("next", "下一步", ConnectionType::Next, 
                                                    BlockDataType::Flow, QPointF(120, 20), QColor(100, 100, 100)));
            break;
    }
    
    return block;
}

QVector<BlockType> BlockFactory::getAvailableBlockTypes()
{
    return {
        BlockType::Start, BlockType::End, BlockType::IfElse, BlockType::Loop, BlockType::ForEach,
        BlockType::Variable, BlockType::Constant, BlockType::Input, BlockType::Output,
        BlockType::Math, BlockType::Logic, BlockType::Compare, BlockType::String,
        BlockType::GetCell, BlockType::SetCell, BlockType::GetRange, BlockType::SetRange,
        BlockType::AddRow, BlockType::DeleteRow, BlockType::Custom
    };
}

QString BlockFactory::getBlockTypeName(BlockType type)
{
    switch (type) {
        case BlockType::Start: return "开始";
        case BlockType::End: return "结束";
        case BlockType::IfElse: return "条件分支";
        case BlockType::Loop: return "循环";
        case BlockType::ForEach: return "遍历";
        case BlockType::Variable: return "变量";
        case BlockType::Constant: return "常量";
        case BlockType::Input: return "输入";
        case BlockType::Output: return "输出";
        case BlockType::Math: return "数学运算";
        case BlockType::Logic: return "逻辑运算";
        case BlockType::Compare: return "比较运算";
        case BlockType::String: return "字符串";
        case BlockType::GetCell: return "获取单元格";
        case BlockType::SetCell: return "设置单元格";
        case BlockType::GetRange: return "获取范围";
        case BlockType::SetRange: return "设置范围";
        case BlockType::AddRow: return "添加行";
        case BlockType::DeleteRow: return "删除行";
        case BlockType::Custom: return "自定义";
        default: return "未知";
    }
}

QString BlockFactory::getBlockTypeDescription(BlockType type)
{
    switch (type) {
        case BlockType::Start: return "程序开始执行的起点";
        case BlockType::End: return "程序执行的终点";
        case BlockType::IfElse: return "根据条件执行不同的分支";
        case BlockType::Loop: return "重复执行一段代码";
        case BlockType::ForEach: return "遍历集合中的每个元素";
        case BlockType::Variable: return "存储和操作变量";
        case BlockType::Constant: return "定义常量值";
        case BlockType::Input: return "获取用户输入";
        case BlockType::Output: return "输出结果";
        case BlockType::Math: return "执行数学运算";
        case BlockType::Logic: return "执行逻辑运算";
        case BlockType::Compare: return "比较两个值";
        case BlockType::String: return "字符串操作";
        case BlockType::GetCell: return "从Excel获取单元格值";
        case BlockType::SetCell: return "设置Excel单元格值";
        case BlockType::GetRange: return "从Excel获取范围数据";
        case BlockType::SetRange: return "设置Excel范围数据";
        case BlockType::AddRow: return "在Excel中添加行";
        case BlockType::DeleteRow: return "在Excel中删除行";
        case BlockType::Custom: return "自定义功能积木";
        default: return "未知积木类型";
    }
}

QColor BlockFactory::getBlockTypeColor(BlockType type)
{
    switch (type) {
        case BlockType::Start: return QColor(76, 175, 80);      // 绿色
        case BlockType::End: return QColor(244, 67, 54);        // 红色
        case BlockType::IfElse: return QColor(255, 152, 0);     // 橙色
        case BlockType::Loop: return QColor(255, 193, 7);       // 黄色
        case BlockType::ForEach: return QColor(255, 235, 59);   // 浅黄色
        case BlockType::Variable: return QColor(156, 39, 176);  // 紫色
        case BlockType::Constant: return QColor(103, 58, 183);  // 深紫色
        case BlockType::Input: return QColor(33, 150, 243);     // 蓝色
        case BlockType::Output: return QColor(3, 169, 244);     // 浅蓝色
        case BlockType::Math: return QColor(0, 188, 212);       // 青色
        case BlockType::Logic: return QColor(0, 150, 136);      // 蓝绿色
        case BlockType::Compare: return QColor(139, 195, 74);   // 浅绿色
        case BlockType::String: return QColor(205, 220, 57);    // 黄绿色
        case BlockType::GetCell: return QColor(255, 87, 34);    // 深橙色
        case BlockType::SetCell: return QColor(255, 111, 97);   // 浅橙色
        case BlockType::GetRange: return QColor(121, 85, 72);   // 棕色
        case BlockType::DeleteRow: return QColor(158, 158, 158); // 灰色
        case BlockType::Custom: return QColor(96, 125, 139);    // 蓝灰色
        default: return QColor(158, 158, 158);                  // 默认灰色
    }
}
