//
// Created by TinaFlow Team
//

#pragma once

#include "Command.hpp"
#include "CompositeCommand.hpp"
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QDateTime>
#include <stack>
#include <memory>

/**
 * @brief 命令管理器 - 管理撤销/重做栈
 * 
 * 功能：
 * - 执行命令并添加到撤销栈
 * - 撤销和重做操作
 * - 命令合并优化
 * - 历史记录管理
 * - 自动保存点
 */
class CommandManager : public QObject
{
    Q_OBJECT

public:
    static CommandManager& instance()
    {
        static CommandManager instance;
        return instance;
    }
    
    /**
     * @brief 执行命令
     * @param command 要执行的命令
     * @return 是否执行成功
     */
    bool executeCommand(std::unique_ptr<Command> command);
    
    /**
     * @brief 撤销上一个命令
     * @return 是否撤销成功
     */
    bool undo();
    
    /**
     * @brief 重做下一个命令
     * @return 是否重做成功
     */
    bool redo();
    
    /**
     * @brief 检查是否可以撤销
     */
    bool canUndo() const;
    
    /**
     * @brief 检查是否可以重做
     */
    bool canRedo() const;
    
    /**
     * @brief 获取下一个撤销命令的描述
     */
    QString getUndoText() const;
    
    /**
     * @brief 获取下一个重做命令的描述
     */
    QString getRedoText() const;
    
    /**
     * @brief 清除所有历史记录
     */
    void clear();
    
    /**
     * @brief 设置撤销栈的最大大小
     * @param limit 最大命令数量，0表示无限制
     */
    void setUndoLimit(int limit);
    
    /**
     * @brief 获取撤销栈大小
     */
    int getUndoLimit() const { return m_undoLimit; }
    
    /**
     * @brief 获取当前撤销栈中的命令数量
     */
    int getUndoCount() const;
    
    /**
     * @brief 获取当前重做栈中的命令数量
     */
    int getRedoCount() const;
    
    /**
     * @brief 开始宏命令（复合操作）
     * @param description 宏命令描述
     */
    void beginMacro(const QString& description);
    
    /**
     * @brief 结束宏命令
     */
    void endMacro();
    
    /**
     * @brief 检查是否在宏命令中
     */
    bool isInMacro() const { return static_cast<bool>(m_currentMacro); }
    
    /**
     * @brief 启用/禁用命令合并
     * @param enabled 是否启用
     */
    void setMergeEnabled(bool enabled) { m_mergeEnabled = enabled; }
    
    /**
     * @brief 检查命令合并是否启用
     */
    bool isMergeEnabled() const { return m_mergeEnabled; }
    
    /**
     * @brief 设置命令合并超时时间（毫秒）
     * @param timeout 超时时间，0表示禁用超时
     */
    void setMergeTimeout(int timeout);
    
    /**
     * @brief 获取撤销历史
     * @param maxCount 最大返回数量
     */
    QStringList getUndoHistory(int maxCount = 10) const;
    
    /**
     * @brief 获取重做历史
     * @param maxCount 最大返回数量
     */
    QStringList getRedoHistory(int maxCount = 10) const;
    
    /**
     * @brief 创建保存点
     * @param name 保存点名称
     */
    void createSavePoint(const QString& name = "");
    
    /**
     * @brief 检查是否有未保存的更改
     */
    bool hasUnsavedChanges() const;
    
    /**
     * @brief 标记为已保存
     */
    void markAsSaved();

signals:
    /**
     * @brief 撤销/重做状态改变
     */
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    
    /**
     * @brief 撤销/重做状态统一改变信号（避免重入问题）
     */
    void undoRedoStateChanged(bool canUndo, bool canRedo);
    
    /**
     * @brief 撤销/重做文本改变
     */
    void undoTextChanged(const QString& text);
    void redoTextChanged(const QString& text);
    
    /**
     * @brief 命令执行相关信号
     */
    void commandExecuted(const QString& description);
    void commandUndone(const QString& description);
    void commandRedone(const QString& description);
    
    /**
     * @brief 历史记录改变
     */
    void historyChanged();
    
    /**
     * @brief 保存状态改变
     */
    void saveStateChanged(bool hasUnsavedChanges);

private slots:
    void onMergeTimeout();

private:
    explicit CommandManager(QObject* parent = nullptr);
    ~CommandManager() = default;
    
    // 禁用拷贝
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    
    void updateSignals();
    void trimUndoStack();
    bool tryMergeCommand(Command* command);
    void clearRedoStack();
    
private:
    std::stack<std::unique_ptr<Command>> m_undoStack;
    std::stack<std::unique_ptr<Command>> m_redoStack;
    
    std::unique_ptr<MacroCommand> m_currentMacro;
    
    int m_undoLimit = 100;  // 默认最多保存100个命令
    bool m_mergeEnabled = true;
    int m_mergeTimeout = 1000;  // 1秒合并超时
    
    QTimer* m_mergeTimer;
    mutable QMutex m_mutex;
    
    int m_savePointIndex = 0;  // 保存点在撤销栈中的位置
    bool m_hasUnsavedChanges = false;
};

/**
 * @brief 宏命令作用域助手 - RAII方式管理宏命令
 */
class MacroCommandScope
{
public:
    MacroCommandScope(const QString& description)
    {
        CommandManager::instance().beginMacro(description);
    }
    
    ~MacroCommandScope()
    {
        CommandManager::instance().endMacro();
    }
    
    // 禁用拷贝和移动
    MacroCommandScope(const MacroCommandScope&) = delete;
    MacroCommandScope& operator=(const MacroCommandScope&) = delete;
    MacroCommandScope(MacroCommandScope&&) = delete;
    MacroCommandScope& operator=(MacroCommandScope&&) = delete;
};

/**
 * @brief 宏命令宏定义
 */
#define BEGIN_MACRO_COMMAND(description) \
    CommandManager::instance().beginMacro(description)

#define END_MACRO_COMMAND() \
    CommandManager::instance().endMacro()

#define SCOPED_MACRO_COMMAND(description) \
    MacroCommandScope macroScope(description)
