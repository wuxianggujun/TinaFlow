#include "widget/ModernToolBar.hpp"
#include "mainwindow.hpp"
#include <QGroupBox>
#include <QSplitter>
#include <QFrame>
#include <QApplication>
#include <QStyle>

ModernToolBar::ModernToolBar(MainWindow* parent)
    : QToolBar(parent)
    , m_mainWindow(parent)
    , m_isRunning(false)
{
    setObjectName("ModernToolBar");
    setWindowTitle(tr("工具栏"));
    setMovable(false);
    setFloatable(false);
    
    setupLayout();
    createFileGroup();
    createEditGroup();
    createExecutionGroup();
    createViewGroup();
    setupStyles();
}

void ModernToolBar::setupLayout()
{
    // QToolBar不需要手动设置布局，它有自己的布局管理
    setFixedHeight(48);
    setMinimumWidth(800);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void ModernToolBar::createFileGroup()
{
    // 创建文件操作动作
    auto* newAction = createAction("new", "新建", "📄", "创建新的流程文件 (Ctrl+N)", QKeySequence::New);
    auto* openAction = createAction("open", "打开", "📁", "打开现有流程文件 (Ctrl+O)", QKeySequence::Open);
    auto* saveAction = createAction("save", "保存", "💾", "保存当前流程文件 (Ctrl+S)", QKeySequence::Save);
    
    // 添加动作到工具栏
    addAction(newAction);
    addAction(openAction);
    addAction(saveAction);
    
    // 连接信号
    connect(newAction, &QAction::triggered, this, &ModernToolBar::newFileRequested);
    connect(openAction, &QAction::triggered, this, &ModernToolBar::openFileRequested);
    connect(saveAction, &QAction::triggered, this, &ModernToolBar::saveFileRequested);
    
    addSeparator();
}

void ModernToolBar::createEditGroup()
{
    // 创建编辑操作动作
    auto* undoAction = createAction("undo", "撤销", "↶", "撤销上一个操作 (Ctrl+Z)", QKeySequence::Undo);
    auto* redoAction = createAction("redo", "重做", "↷", "重做下一个操作 (Ctrl+Y)", QKeySequence::Redo);
    
    // 默认禁用
    undoAction->setEnabled(false);
    redoAction->setEnabled(false);
    
    // 添加动作到工具栏
    addAction(undoAction);
    addAction(redoAction);
    
    // 连接信号
    connect(undoAction, &QAction::triggered, this, &ModernToolBar::undoRequested);
    connect(redoAction, &QAction::triggered, this, &ModernToolBar::redoRequested);
    
    addSeparator();
}

void ModernToolBar::createExecutionGroup()
{
    // 创建执行控制动作
    auto* runAction = createAction("run", "运行", "▶️", "开始执行流程 (F5)", QKeySequence("F5"));
    auto* pauseAction = createAction("pause", "暂停", "⏸️", "暂停执行 (F6)", QKeySequence("F6"));
    auto* stopAction = createAction("stop", "停止", "⏹️", "停止执行 (F7)", QKeySequence("F7"));
    
    // 设置初始状态
    pauseAction->setEnabled(false);
    stopAction->setEnabled(false);
    
    // 添加动作到工具栏
    addAction(runAction);
    addAction(pauseAction);
    addAction(stopAction);
    
    // 连接信号
    connect(runAction, &QAction::triggered, this, &ModernToolBar::runRequested);
    connect(pauseAction, &QAction::triggered, this, &ModernToolBar::pauseRequested);
    connect(stopAction, &QAction::triggered, this, &ModernToolBar::stopRequested);
    
    addSeparator();
}

void ModernToolBar::createViewGroup()
{
    // 创建视图控制动作
    auto* zoomFitAction = createAction("zoomFit", "适应", "🔍", "缩放以适应所有节点 (Ctrl+0)", QKeySequence("Ctrl+0"));
    auto* zoomInAction = createAction("zoomIn", "放大", "+", "放大视图 (Ctrl++)", QKeySequence::ZoomIn);
    auto* zoomOutAction = createAction("zoomOut", "缩小", "-", "缩小视图 (Ctrl+-)", QKeySequence::ZoomOut);
    
    // 添加动作到工具栏
    addAction(zoomFitAction);
    addAction(zoomInAction);
    addAction(zoomOutAction);
    
    // 连接信号
    connect(zoomFitAction, &QAction::triggered, this, &ModernToolBar::zoomFitRequested);
    connect(zoomInAction, &QAction::triggered, this, &ModernToolBar::zoomInRequested);
    connect(zoomOutAction, &QAction::triggered, this, &ModernToolBar::zoomOutRequested);
}



void ModernToolBar::setupStyles()
{
    setStyleSheet(
        "ModernToolBar {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                               stop:0 #f8f8f8, stop:1 #e8e8e8);"
        "    border-bottom: 1px solid #c0c0c0;"
        "}"
        "QToolButton {"
        "    background-color: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"
        "    margin: 1px;"
        "    font-size: 11px;"
        "}"
        "QToolButton:hover {"
        "    background-color: rgba(0,0,0,0.1);"
        "    border: 1px solid #999;"
        "}"
        "QToolButton:pressed {"
        "    background-color: rgba(0,0,0,0.2);"
        "}"
        "QToolButton:checked {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: 1px solid #1976D2;"
        "}"
    );
}

// addToolBarSeparator方法已移除，直接使用addSeparator()

QAction* ModernToolBar::createAction(const QString& name, const QString& text, 
                                   const QString& icon, const QString& tooltip, 
                                   const QKeySequence& shortcut)
{
    auto* action = new QAction(QString("%1 %2").arg(icon, text), this);
    action->setToolTip(tooltip);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }
    
    m_actions[name] = action;
    return action;
}

// createToolButton方法已移除，直接使用QAction

void ModernToolBar::updateExecutionState(bool running)
{
    m_isRunning = running;
    
    // 更新按钮状态
    m_actions["run"]->setEnabled(!running);
    m_actions["pause"]->setEnabled(running);
    m_actions["stop"]->setEnabled(running);
}



void ModernToolBar::updateUndoRedoState(bool canUndo, bool canRedo)
{
    m_actions["undo"]->setEnabled(canUndo);
    m_actions["redo"]->setEnabled(canRedo);
}



QAction* ModernToolBar::getAction(const QString& name) const
{
    return m_actions.value(name, nullptr);
}

void ModernToolBar::onRecentFileTriggered()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (action) {
        emit recentFileRequested(action->data().toString());
    }
}

 
