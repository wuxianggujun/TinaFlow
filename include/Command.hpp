//
// Created by TinaFlow Team
//

#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDateTime>
#include <QMap>
#include <functional>
#include <memory>

/**
 * @brief 命令接口 - 实现命令模式
 * 
 * 所有可撤销的操作都应该实现这个接口
 */
class Command
{
public:
    virtual ~Command() = default;
    
    /**
     * @brief 执行命令
     * @return 是否执行成功
     */
    virtual bool execute() = 0;
    
    /**
     * @brief 撤销命令
     * @return 是否撤销成功
     */
    virtual bool undo() = 0;
    
    /**
     * @brief 重做命令（默认实现为再次执行）
     * @return 是否重做成功
     */
    virtual bool redo()
    {
        return execute();
    }
    
    /**
     * @brief 获取命令描述（用于UI显示）
     */
    virtual QString getDescription() const = 0;
    
    /**
     * @brief 获取命令类型
     */
    virtual QString getType() const = 0;
    
    /**
     * @brief 获取命令ID
     */
    virtual QUuid getId() const { return m_id; }
    
    /**
     * @brief 检查命令是否可以与其他命令合并
     * @param other 其他命令
     * @return 是否可以合并
     */
    virtual bool canMergeWith(const Command* other) const
    {
        Q_UNUSED(other);
        return false;
    }
    
    /**
     * @brief 与其他命令合并
     * @param other 其他命令
     * @return 是否合并成功
     */
    virtual bool mergeWith(const Command* other)
    {
        Q_UNUSED(other);
        return false;
    }
    
    /**
     * @brief 序列化命令到JSON
     */
    virtual QJsonObject toJson() const
    {
        QJsonObject json;
        json["id"] = m_id.toString();
        json["type"] = getType();
        json["description"] = getDescription();
        json["timestamp"] = m_timestamp.toString(Qt::ISODate);
        return json;
    }
    
    /**
     * @brief 从JSON反序列化命令
     */
    virtual bool fromJson(const QJsonObject& json)
    {
        if (json.contains("id")) {
            m_id = QUuid::fromString(json["id"].toString());
        }
        if (json.contains("timestamp")) {
            m_timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
        }
        return true;
    }
    
    /**
     * @brief 获取命令创建时间
     */
    QDateTime getTimestamp() const { return m_timestamp; }

protected:
    Command() : m_id(QUuid::createUuid()), m_timestamp(QDateTime::currentDateTime()) {}
    
private:
    QUuid m_id;
    QDateTime m_timestamp;
};

// CompositeCommand 和 MacroCommand 已移动到单独的头文件
// #include "CompositeCommand.hpp" 如果需要使用

/**
 * @brief 命令工厂 - 用于创建和注册命令类型
 */
class CommandFactory
{
public:
    using CreateFunction = std::function<std::unique_ptr<Command>()>;
    
    static CommandFactory& instance()
    {
        static CommandFactory instance;
        return instance;
    }
    
    void registerCommand(const QString& type, CreateFunction createFunc)
    {
        m_creators[type] = createFunc;
    }
    
    std::unique_ptr<Command> createCommand(const QString& type)
    {
        auto it = m_creators.find(type);
        if (it != m_creators.end()) {
            return it.value()();
        }
        return nullptr;
    }
    
    QStringList getRegisteredTypes() const
    {
        QStringList types;
        for (auto it = m_creators.begin(); it != m_creators.end(); ++it) {
            types.append(it.key());
        }
        return types;
    }

private:
    QMap<QString, CreateFunction> m_creators;
};

/**
 * @brief 命令注册宏
 */
#define REGISTER_COMMAND(CommandClass) \
    static bool registered_##CommandClass = []() { \
        CommandFactory::instance().registerCommand( \
            #CommandClass, \
            []() -> std::unique_ptr<Command> { \
                return std::make_unique<CommandClass>(); \
            } \
        ); \
        return true; \
    }();
