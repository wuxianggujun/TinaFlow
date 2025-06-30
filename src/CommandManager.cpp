#include "CommandManager.hpp"
#include "CompositeCommand.hpp"
#include <QDebug>
#include <QMutexLocker>
#include <deque>

CommandManager::CommandManager(QObject* parent)
    : QObject(parent)
    , m_mergeTimer(new QTimer(this))
{
    m_mergeTimer->setSingleShot(true);
    connect(m_mergeTimer, &QTimer::timeout, this, &CommandManager::onMergeTimeout);
    
    qDebug() << "CommandManager: Initialized";
}

bool CommandManager::executeCommand(std::unique_ptr<Command> command)
{
    if (!command) {
        qWarning() << "CommandManager: Attempted to execute null command";
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "CommandManager: Executing command:" << command->getDescription();
    
    // 如果在宏命令中，添加到当前宏
    if (m_currentMacro) {
        if (command->execute()) {
            m_currentMacro->addCommand(std::move(command));
            return true;
        } else {
            qWarning() << "CommandManager: Command execution failed in macro";
            return false;
        }
    }
    
    // 尝试与上一个命令合并
    if (m_mergeEnabled && !m_undoStack.empty() && m_mergeTimer->remainingTime() > 0) {
        if (tryMergeCommand(command.get())) {
            qDebug() << "CommandManager: Command merged with previous command";
            // 重新启动合并计时器
            if (m_mergeTimeout > 0) {
                m_mergeTimer->start(m_mergeTimeout);
            }
            return true;
        }
    }
    
    // 执行命令
    if (!command->execute()) {
        qWarning() << "CommandManager: Command execution failed:" << command->getDescription();
        return false;
    }
    
    // 清除重做栈
    clearRedoStack();
    
    // 保存命令描述
    QString commandDescription = command->getDescription();

    // 添加到撤销栈
    m_undoStack.push(std::move(command));

    // 限制撤销栈大小
    trimUndoStack();

    // 标记有未保存的更改
    m_hasUnsavedChanges = true;

    // 启动合并计时器
    if (m_mergeEnabled && m_mergeTimeout > 0) {
        m_mergeTimer->start(m_mergeTimeout);
    }

    // 重新启用信号，但优化发射顺序避免循环
    updateSignals();

    // 只发射必要的信号
    emit commandExecuted(commandDescription);
    emit saveStateChanged(m_hasUnsavedChanges);
    
    // historyChanged信号单独发射，避免与其他信号冲突
    QMetaObject::invokeMethod(this, [this]() {
        emit historyChanged();
    }, Qt::QueuedConnection);
    
    return true;
}

bool CommandManager::undo()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_undoStack.empty()) {
        qDebug() << "CommandManager: Nothing to undo";
        return false;
    }

    auto command = std::move(const_cast<std::unique_ptr<Command>&>(m_undoStack.top()));
    m_undoStack.pop();
    
    qDebug() << "CommandManager: Undoing command:" << command->getDescription();
    
    if (command->undo()) {
        QString description = command->getDescription();
        m_redoStack.push(std::move(command));
        
        // 检查是否回到保存点
        if (static_cast<int>(m_undoStack.size()) == m_savePointIndex) {
            m_hasUnsavedChanges = false;
        } else {
            m_hasUnsavedChanges = true;
        }

        updateSignals();

        emit commandUndone(description);
        emit historyChanged();
        emit saveStateChanged(m_hasUnsavedChanges);
        
        return true;
    } else {
        qWarning() << "CommandManager: Undo failed, restoring command to stack";
        m_undoStack.push(std::move(command));
        return false;
    }
}

bool CommandManager::redo()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_redoStack.empty()) {
        qDebug() << "CommandManager: Nothing to redo";
        return false;
    }

    auto command = std::move(const_cast<std::unique_ptr<Command>&>(m_redoStack.top()));
    m_redoStack.pop();
    
    qDebug() << "CommandManager: Redoing command:" << command->getDescription();
    
    if (command->redo()) {
        QString description = command->getDescription();
        m_undoStack.push(std::move(command));
        m_hasUnsavedChanges = true;
        
        updateSignals();
        
        emit commandRedone(description);
        emit historyChanged();
        emit saveStateChanged(m_hasUnsavedChanges);
        
        return true;
    } else {
        qWarning() << "CommandManager: Redo failed, restoring command to stack";
        m_redoStack.push(std::move(command));
        return false;
    }
}

bool CommandManager::canUndo() const
{
    QMutexLocker locker(&m_mutex);
    return !m_undoStack.empty();
}

bool CommandManager::canRedo() const
{
    QMutexLocker locker(&m_mutex);
    return !m_redoStack.empty();
}

QString CommandManager::getUndoText() const
{
    QMutexLocker locker(&m_mutex);
    if (m_undoStack.empty()) {
        return QString();
    }
    return QString("撤销 %1").arg(m_undoStack.top()->getDescription());
}

QString CommandManager::getRedoText() const
{
    QMutexLocker locker(&m_mutex);
    if (m_redoStack.empty()) {
        return QString();
    }
    return QString("重做 %1").arg(m_redoStack.top()->getDescription());
}

void CommandManager::clear()
{
    QMutexLocker locker(&m_mutex);

    qDebug() << "CommandManager: Clearing all history";

    // std::stack没有clear()方法，需要手动清空
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }
    while (!m_redoStack.empty()) {
        m_redoStack.pop();
    }

    m_currentMacro.reset();
    m_savePointIndex = 0;
    m_hasUnsavedChanges = false;

    // 停止合并计时器
    m_mergeTimer->stop();

    updateSignals();

    emit historyChanged();
    emit saveStateChanged(m_hasUnsavedChanges);
}

void CommandManager::setUndoLimit(int limit)
{
    QMutexLocker locker(&m_mutex);
    
    m_undoLimit = limit;
    trimUndoStack();
    
    qDebug() << "CommandManager: Undo limit set to" << limit;
}

int CommandManager::getUndoCount() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<int>(m_undoStack.size());
}

int CommandManager::getRedoCount() const
{
    QMutexLocker locker(&m_mutex);
    return static_cast<int>(m_redoStack.size());
}

void CommandManager::beginMacro(const QString& description)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_currentMacro) {
        qWarning() << "CommandManager: Already in macro, ignoring beginMacro";
        return;
    }
    
    m_currentMacro = std::make_unique<MacroCommand>(description);
    qDebug() << "CommandManager: Started macro:" << description;
}

void CommandManager::endMacro()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_currentMacro) {
        qWarning() << "CommandManager: Not in macro, ignoring endMacro";
        return;
    }
    
    qDebug() << "CommandManager: Ending macro:" << m_currentMacro->getDescription();
    
    // 如果宏命令不为空，添加到撤销栈
    if (!m_currentMacro->isEmpty()) {
        // 清除重做栈
        clearRedoStack();
        
        QString description = m_currentMacro->getDescription();
        
        // 添加到撤销栈
        m_undoStack.push(std::move(m_currentMacro));
        
        // 限制撤销栈大小
        trimUndoStack();
        
        // 标记有未保存的更改
        m_hasUnsavedChanges = true;
        
        updateSignals();
        
        emit commandExecuted(description);
        emit historyChanged();
        emit saveStateChanged(m_hasUnsavedChanges);
    }
    
    m_currentMacro.reset();
}

void CommandManager::setMergeTimeout(int timeout)
{
    m_mergeTimeout = timeout;
    qDebug() << "CommandManager: Merge timeout set to" << timeout << "ms";
}

QStringList CommandManager::getUndoHistory(int maxCount) const
{
    QMutexLocker locker(&m_mutex);

    QStringList history;
    
    // 简化实现：只返回当前可撤销的命令描述
    if (!m_undoStack.empty() && maxCount > 0) {
        history.append(m_undoStack.top()->getDescription());
    }

    return history;
}

QStringList CommandManager::getRedoHistory(int maxCount) const
{
    QMutexLocker locker(&m_mutex);

    QStringList history;
    
    // 简化实现：只返回当前可重做的命令描述
    if (!m_redoStack.empty() && maxCount > 0) {
        history.append(m_redoStack.top()->getDescription());
    }

    return history;
}

void CommandManager::createSavePoint(const QString& name)
{
    QMutexLocker locker(&m_mutex);
    
    m_savePointIndex = static_cast<int>(m_undoStack.size());
    m_hasUnsavedChanges = false;
    
    qDebug() << "CommandManager: Save point created:" << name << "at index" << m_savePointIndex;
    
    emit saveStateChanged(m_hasUnsavedChanges);
}

bool CommandManager::hasUnsavedChanges() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasUnsavedChanges;
}

void CommandManager::markAsSaved()
{
    createSavePoint("Auto Save");
}

void CommandManager::onMergeTimeout()
{
    // 合并超时，停止尝试合并后续命令
    qDebug() << "CommandManager: Merge timeout expired";
}

void CommandManager::updateSignals()
{
    bool canUndoNow = !m_undoStack.empty();
    bool canRedoNow = !m_redoStack.empty();
    
    // 直接计算文本，避免重复加锁
    QString undoText;
    QString redoText;
    
    if (!m_undoStack.empty()) {
        undoText = QString("撤销 %1").arg(m_undoStack.top()->getDescription());
    }
    
    if (!m_redoStack.empty()) {
        redoText = QString("重做 %1").arg(m_redoStack.top()->getDescription());
    }

    // 发出统一的状态改变信号（避免重入问题）
    emit undoRedoStateChanged(canUndoNow, canRedoNow);
    emit undoTextChanged(undoText);
    emit redoTextChanged(redoText);
}

void CommandManager::trimUndoStack()
{
    if (m_undoLimit <= 0) return;

    int currentSize = static_cast<int>(m_undoStack.size());
    if (currentSize <= m_undoLimit) return;

    // 使用deque进行高效的操作
    std::deque<std::unique_ptr<Command>> commands;
    
    // 将所有命令移动到deque中
    while (!m_undoStack.empty()) {
        commands.push_front(std::move(const_cast<std::unique_ptr<Command>&>(m_undoStack.top())));
        m_undoStack.pop();
    }
    
    // 计算需要删除的命令数量
    int toRemove = currentSize - m_undoLimit;
    
    // 从前面删除旧命令
    for (int i = 0; i < toRemove; ++i) {
        commands.pop_front();
        // 调整保存点索引
        if (m_savePointIndex > 0) {
            m_savePointIndex--;
        } else {
            // 如果保存点被删除，标记为有未保存更改
            m_hasUnsavedChanges = true;
        }
    }
    
    // 将剩余命令重新放回栈中
    for (auto it = commands.rbegin(); it != commands.rend(); ++it) {
        m_undoStack.push(std::move(*it));
    }

    qDebug() << "CommandManager: Trimmed undo stack from" << currentSize << "to" << m_undoStack.size() << "commands";
}

bool CommandManager::tryMergeCommand(Command* command)
{
    if (m_undoStack.empty()) {
        return false;
    }

    Command* lastCommand = m_undoStack.top().get();
    if (lastCommand->canMergeWith(command)) {
        if (lastCommand->mergeWith(command)) {
            qDebug() << "CommandManager: Successfully merged commands";
            return true;
        }
    }

    return false;
}

void CommandManager::clearRedoStack()
{
    if (!m_redoStack.empty()) {
        while (!m_redoStack.empty()) {
            m_redoStack.pop();
        }
        qDebug() << "CommandManager: Redo stack cleared";
    }
}
