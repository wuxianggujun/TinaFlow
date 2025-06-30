#include "CompositeCommand.hpp"
#include <QJsonArray>

CompositeCommand::CompositeCommand(const QString& description) 
    : m_description(description) 
{
}

void CompositeCommand::addCommand(std::unique_ptr<Command> command)
{
    m_commands.push_back(std::move(command));
}

bool CompositeCommand::execute()
{
    bool success = true;
    for (auto& command : m_commands) {
        if (!command->execute()) {
            success = false;
            // 如果执行失败，撤销已执行的命令
            for (int i = static_cast<int>(m_commands.size()) - 1; i >= 0; --i) {
                if (m_commands[i].get() != command.get()) {
                    m_commands[i]->undo();
                } else {
                    break;
                }
            }
            break;
        }
    }
    return success;
}

bool CompositeCommand::undo()
{
    bool success = true;
    // 逆序撤销
    for (int i = static_cast<int>(m_commands.size()) - 1; i >= 0; --i) {
        if (!m_commands[i]->undo()) {
            success = false;
        }
    }
    return success;
}

bool CompositeCommand::redo()
{
    return execute();
}

QString CompositeCommand::getDescription() const
{
    return m_description;
}

QString CompositeCommand::getType() const
{
    return "CompositeCommand";
}

size_t CompositeCommand::getCommandCount() const
{
    return m_commands.size();
}

bool CompositeCommand::isEmpty() const
{
    return m_commands.empty();
}

QJsonObject CompositeCommand::toJson() const
{
    QJsonObject json = Command::toJson();
    json["description"] = m_description;
    
    QJsonArray commandsArray;
    for (const auto& command : m_commands) {
        commandsArray.append(command->toJson());
    }
    json["commands"] = commandsArray;
    
    return json;
}

MacroCommand::MacroCommand(const QString& description) 
    : CompositeCommand(description) 
{
}

QString MacroCommand::getType() const
{
    return "MacroCommand";
} 