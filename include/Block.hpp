#pragma once

#include <QString>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QUuid>
#include <memory>

/**
 * @brief 积木类型枚举
 */
enum class BlockType {
    // 控制流积木
    Start,          // 开始
    End,            // 结束
    IfElse,         // 条件分支
    Loop,           // 循环
    ForEach,        // 遍历
    
    // 数据积木
    Variable,       // 变量
    Constant,       // 常量
    Input,          // 输入
    Output,         // 输出
    
    // 运算积木
    Math,           // 数学运算
    Logic,          // 逻辑运算
    Compare,        // 比较运算
    String,         // 字符串操作
    
    // Excel操作积木
    GetCell,        // 获取单元格
    SetCell,        // 设置单元格
    GetRange,       // 获取范围
    SetRange,       // 设置范围
    AddRow,         // 添加行
    DeleteRow,      // 删除行
    
    // 自定义积木
    Custom          // 自定义
};

/**
 * @brief 连接点类型枚举
 */
enum class ConnectionType {
    Input,          // 输入连接点
    Output,         // 输出连接点
    Next,           // 下一步连接点
    Previous        // 上一步连接点
};

/**
 * @brief 积木数据类型枚举
 */
enum class BlockDataType {
    Any,            // 任意类型
    Number,         // 数字
    String,         // 字符串
    Boolean,        // 布尔值
    Array,          // 数组
    Object,         // 对象
    Flow            // 控制流
};

/**
 * @brief 连接点结构
 */
struct ConnectionPoint {
    QString id;                     // 连接点ID
    QString name;                   // 连接点名称
    ConnectionType type;            // 连接点类型
    BlockDataType dataType;         // 数据类型
    QPointF position;               // 相对于积木的位置
    QColor color;                   // 连接点颜色
    bool isConnected = false;       // 是否已连接
    QString connectedBlockId;       // 连接的积木ID
    QString connectedPointId;       // 连接的连接点ID
    
    ConnectionPoint() = default;
    ConnectionPoint(const QString& id, const QString& name, ConnectionType type, 
                   BlockDataType dataType, const QPointF& position, const QColor& color)
        : id(id), name(name), type(type), dataType(dataType), position(position), color(color) {}
};

/**
 * @brief 积木连接结构
 */
struct BlockConnection {
    QString id;                     // 连接ID
    QString sourceBlockId;          // 源积木ID
    QString sourcePointId;          // 源连接点ID
    QString targetBlockId;          // 目标积木ID
    QString targetPointId;          // 目标连接点ID
    QColor color;                   // 连接线颜色
    
    BlockConnection() = default;
    BlockConnection(const QString& id, const QString& sourceBlockId, const QString& sourcePointId,
                   const QString& targetBlockId, const QString& targetPointId, const QColor& color)
        : id(id), sourceBlockId(sourceBlockId), sourcePointId(sourcePointId)
        , targetBlockId(targetBlockId), targetPointId(targetPointId), color(color) {}
};

/**
 * @brief 积木基类
 */
class Block {
public:
    explicit Block(const QString& id = QString(), BlockType type = BlockType::Custom, const QString& title = QString());
    virtual ~Block() = default;

    // 基本属性
    QString getId() const { return m_id; }
    void setId(const QString& id) { m_id = id; }
    
    BlockType getType() const { return m_type; }
    void setType(BlockType type) { m_type = type; }
    
    QString getTitle() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }
    
    QString getDescription() const { return m_description; }
    void setDescription(const QString& description) { m_description = description; }
    
    // 位置和大小
    QPointF getPosition() const { return m_position; }
    void setPosition(const QPointF& position) { m_position = position; }
    
    QRectF getBounds() const { return m_bounds; }
    void setBounds(const QRectF& bounds) { m_bounds = bounds; }
    
    // 外观
    QColor getColor() const { return m_color; }
    void setColor(const QColor& color) { m_color = color; }
    
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    // 连接点管理
    void addConnectionPoint(const ConnectionPoint& point);
    void removeConnectionPoint(const QString& pointId);
    ConnectionPoint* getConnectionPoint(const QString& pointId);
    const QVector<ConnectionPoint>& getConnectionPoints() const { return m_connectionPoints; }
    
    // 参数管理
    void setParameter(const QString& key, const QVariant& value);
    QVariant getParameter(const QString& key) const;
    const QJsonObject& getParameters() const { return m_parameters; }
    
    // 序列化
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject& json);

protected:
    QString m_id;                           // 积木ID
    BlockType m_type;                       // 积木类型
    QString m_title;                        // 积木标题
    QString m_description;                  // 积木描述
    QPointF m_position;                     // 积木位置
    QRectF m_bounds;                        // 积木边界
    QColor m_color;                         // 积木颜色
    bool m_selected = false;                // 是否选中
    bool m_enabled = true;                  // 是否启用
    QVector<ConnectionPoint> m_connectionPoints; // 连接点列表
    QJsonObject m_parameters;               // 积木参数
};

/**
 * @brief 积木工厂类
 */
class BlockFactory {
public:
    static std::shared_ptr<Block> createBlock(BlockType type, const QString& id = QString());
    static QVector<BlockType> getAvailableBlockTypes();
    static QString getBlockTypeName(BlockType type);
    static QString getBlockTypeDescription(BlockType type);
    static QColor getBlockTypeColor(BlockType type);
};
