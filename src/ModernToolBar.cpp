#include "widget/ModernToolBar.hpp"
#include "mainwindow.hpp"
#include "IconManager.hpp"
#include <QGroupBox>
#include <QSplitter>
#include <QFrame>
#include <QApplication>
#include <QStyle>

ModernToolBar::ModernToolBar(MainWindow* parent, bool showFileActions)
    : QToolBar(parent)
    , m_mainWindow(parent)
    , m_isRunning(false)
    , m_isDebugging(false)
{
    setObjectName("ModernToolBar");
    setWindowTitle(tr("工具栏"));
    setMovable(false);
    setFloatable(false);

    // 设置工具栏样式
    setToolButtonStyle(Qt::ToolButtonIconOnly); // 全局设置只显示图标
    setIconSize(QSize(20, 20)); // 设置图标大小

    setupLayout();
    if (showFileActions) {
        createFileGroup();
        addSeparator();
    }
    createEditGroup();
    createExecutionGroup();
    createViewGroup();
    setupStyles();
}

void ModernToolBar::setupLayout()
{
    // QToolBar不需要手动设置布局，它有自己的布局管理
    setFixedHeight(32); // 更紧凑的高度
    setMinimumWidth(800);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

void ModernToolBar::createFileGroup()
{
    // 创建文件操作动作 - 使用图标管理器
    auto* newAction = createAction("new", "", "", "创建新的流程文件 (Ctrl+N)", QKeySequence::New);
    newAction->setIcon(Icons::get(IconType::FilePlus, IconSize::Small));

    auto* openAction = createAction("open", "", "", "打开现有流程文件 (Ctrl+O)", QKeySequence::Open);
    openAction->setIcon(Icons::get(IconType::Folder, IconSize::Small));

    auto* saveAction = createAction("save", "", "", "保存当前流程文件 (Ctrl+S)", QKeySequence::Save);
    saveAction->setIcon(Icons::get(IconType::Save, IconSize::Small));
    
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
    // 创建编辑操作动作 - 使用正确的图标
    auto* undoAction = createAction("undo", "", "", "撤销上一个操作 (Ctrl+Z)", QKeySequence::Undo);
    undoAction->setIcon(Icons::get(IconType::Undo, IconSize::Small));

    auto* redoAction = createAction("redo", "", "", "重做下一个操作 (Ctrl+Y)", QKeySequence::Redo);
    redoAction->setIcon(Icons::get(IconType::Redo, IconSize::Small));
    
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
    // 执行控制组 - 样式已在构造函数中设置

    // 创建执行控制动作 - 使用图标管理器，不显示文本
    auto* runAction = createAction("run", "", "", "开始执行流程 (F5)", QKeySequence("F5"));
    runAction->setIcon(Icons::get(IconType::Play, IconSize::Small));

    auto* debugAction = createAction("debug", "", "", "调试执行流程 (F6)", QKeySequence("F6"));
    debugAction->setIcon(Icons::get(IconType::Bug, IconSize::Small));

    auto* pauseAction = createAction("pause", "", "", "暂停执行 (F7)", QKeySequence("F7"));
    pauseAction->setIcon(Icons::get(IconType::Pause, IconSize::Small));

    auto* stopAction = createAction("stop", "", "", "停止执行 (F8)", QKeySequence("F8"));
    stopAction->setIcon(Icons::get(IconType::Pause, IconSize::Small));

    // 设置初始状态 - 空闲时显示运行和调试按钮
    runAction->setEnabled(true);
    runAction->setVisible(true);
    debugAction->setEnabled(true);
    debugAction->setVisible(true);
    pauseAction->setEnabled(false);
    pauseAction->setVisible(false);
    stopAction->setEnabled(false);
    stopAction->setVisible(false);

    // 添加动作到工具栏
    addAction(runAction);
    addAction(debugAction);
    addAction(pauseAction);
    addAction(stopAction);

    // 连接信号
    connect(runAction, &QAction::triggered, this, &ModernToolBar::runRequested);
    connect(debugAction, &QAction::triggered, this, &ModernToolBar::debugRequested);
    connect(pauseAction, &QAction::triggered, this, &ModernToolBar::stopRequested); // 暂停实际上是停止
    connect(stopAction, &QAction::triggered, this, &ModernToolBar::stopRequested);

    addSeparator();
}

void ModernToolBar::createViewGroup()
{
    // 创建视图控制动作 - 使用正确的图标
    auto* zoomFitAction = createAction("zoomFit", "", "", "缩放以适应所有节点 (Ctrl+0)", QKeySequence("Ctrl+0"));
    zoomFitAction->setIcon(Icons::get(IconType::Maximize, IconSize::Small));

    auto* zoomInAction = createAction("zoomIn", "", "", "放大视图 (Ctrl++)", QKeySequence::ZoomIn);
    zoomInAction->setIcon(Icons::get(IconType::ZoomIn, IconSize::Small));

    auto* zoomOutAction = createAction("zoomOut", "", "", "缩小视图 (Ctrl+-)", QKeySequence::ZoomOut);
    zoomOutAction->setIcon(Icons::get(IconType::ZoomOut, IconSize::Small));
    
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
        "    spacing: 2px;"
        "}"
        "QToolButton {"
        "    background-color: transparent;"
        "    border: 1px solid transparent;"
        "    border-radius: 3px;"
        "    padding: 3px;"  // 减少内边距
        "    margin: 1px;"
        "    min-width: 24px;"  // 设置最小宽度
        "    min-height: 24px;" // 设置最小高度
        "    max-width: 24px;"  // 设置最大宽度
        "    max-height: 24px;" // 设置最大高度
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
        "QToolButton:disabled {"
        "    opacity: 0.5;"
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

void ModernToolBar::updateExecutionState(bool running, bool debugging)
{
    m_isRunning = running;
    m_isDebugging = debugging;

    // 根据状态更新按钮
    if (running || debugging) {
        // 运行中或调试中：禁用运行和调试，显示暂停按钮
        m_actions["run"]->setEnabled(false);
        m_actions["debug"]->setEnabled(false);

        // 显示暂停按钮
        m_actions["pause"]->setVisible(true);
        m_actions["pause"]->setEnabled(true);

        // 更新暂停按钮的工具提示
        if (debugging) {
            m_actions["pause"]->setToolTip("暂停调试执行 (F7)");
        } else {
            m_actions["pause"]->setToolTip("暂停运行执行 (F7)");
        }

        // 隐藏停止按钮
        m_actions["stop"]->setVisible(false);
    } else {
        // 空闲状态：恢复原始状态
        m_actions["run"]->setEnabled(true);
        m_actions["debug"]->setEnabled(true);

        // 隐藏暂停和停止按钮
        m_actions["pause"]->setVisible(false);
        m_actions["pause"]->setEnabled(false);
        m_actions["stop"]->setVisible(false);
        m_actions["stop"]->setEnabled(false);
    }
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



 
