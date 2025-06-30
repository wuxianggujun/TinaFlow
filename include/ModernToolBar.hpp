#pragma once

#include <QWidget>
#include <QToolBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QButtonGroup>
#include <QMenu>

class MainWindow;

/**
 * @brief 现代化工具栏组件
 * 
 * 提供统一风格的工具栏，包含：
 * - 文件操作组
 * - 编辑操作组  
 * - 执行控制组
 * - 视图控制组
 * - 状态信息区
 */
class ModernToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit ModernToolBar(MainWindow* parent = nullptr);
    
    // 状态更新方法
    void updateExecutionState(bool running);
    void updateUndoRedoState(bool canUndo, bool canRedo);
    
    // 获取动作指针
    QAction* getAction(const QString& name) const;

signals:
    // 文件操作信号
    void newFileRequested();
    void openFileRequested();
    void saveFileRequested();
    void recentFileRequested(const QString& filePath);
    
    // 编辑操作信号
    void undoRequested();
    void redoRequested();
    
    // 执行控制信号
    void runRequested();
    void pauseRequested();
    void stopRequested();
    
    // 视图控制信号
    void zoomFitRequested();
    void zoomInRequested();
    void zoomOutRequested();

private slots:
    void onRecentFileTriggered();

private:
    void setupLayout();
    void createFileGroup();
    void createEditGroup();
    void createExecutionGroup();
    void createViewGroup();
    void setupStyles();
    
    QAction* createAction(const QString& name, const QString& text, 
                         const QString& icon, const QString& tooltip, 
                         const QKeySequence& shortcut = QKeySequence());

private:
    MainWindow* m_mainWindow;
    
    // 动作映射
    QMap<QString, QAction*> m_actions;
    
    // 执行状态
    bool m_isRunning;
}; 