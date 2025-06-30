#pragma once

#include "Command.hpp"
#include <vector>
#include <memory>

/**
 * @brief 复合命令 - 包含多个子命令
 */
class CompositeCommand : public Command
{
public:
    CompositeCommand(const QString& description);
    
    void addCommand(std::unique_ptr<Command> command);
    
    bool execute() override;
    bool undo() override;
    bool redo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    size_t getCommandCount() const;
    bool isEmpty() const;
    
    QJsonObject toJson() const override;

private:
    QString m_description;
    std::vector<std::unique_ptr<Command>> m_commands;
};

/**
 * @brief 宏命令 - 将多个操作组合成一个可撤销的单元
 */
class MacroCommand : public CompositeCommand
{
public:
    MacroCommand(const QString& description);
    QString getType() const override;
}; 