//
// Created by wuxianggujun on 25-7-1.
//

#pragma once

// ADS库头文件
#include <DockManager.h>
#include <DockWidget.h>
#include <DockAreaWidget.h>

#include <QObject>
#include <QMainWindow>
#include <QHash>
#include <QString>
#include <QJsonObject>
#include <QDateTime>

class NodePalette;
class CommandHistoryWidget;

/**
 * @brief ADS面板管理器 - 统一管理所有停靠面板
 * 
 * 这个类负责：
 * 1. 管理所有面板的创建、销毁和状态
 * 2. 提供统一的API接口
 * 3. 处理布局预设和状态保存/恢复
 * 4. 协调面板之间的交互
 */
class ADSPanelManager : public QObject
{
    Q_OBJECT

public:
    explicit ADSPanelManager(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~ADSPanelManager();

    // 面板类型枚举
    enum PanelType {
        PropertyPanel,
        NodePalettePanel,  // 重命名避免与类名冲突
        CommandHistory,
        OutputConsole,
        ProjectExplorer,
        DebugConsole,
        CustomPanel
    };

    // 初始化和清理
    void initialize();
    void shutdown();

    // 面板创建和管理
    ads::CDockWidget* createPanel(PanelType type, const QString& panelId, const QString& title);
    void removePanel(const QString& panelId);
    ads::CDockWidget* getPanel(const QString& panelId) const;
    QList<ads::CDockWidget*> getAllPanels() const;

    // 专用面板创建方法
    ads::CDockWidget* createPropertyPanel();
    ads::CDockWidget* createNodePalettePanel();
    ads::CDockWidget* createCommandHistoryPanel();
    ads::CDockWidget* createOutputConsolePanel();
    ads::CDockWidget* createProjectExplorerPanel();

    // 布局管理
    void setupDefaultLayout();

    // 布局预设
    void saveLayoutPreset(const QString& name);
    void loadLayoutPreset(const QString& name);
    void deleteLayoutPreset(const QString& name);
    QStringList getLayoutPresets() const;

    // 状态管理
    QJsonObject saveState() const;
    void restoreState(const QJsonObject& state);

    // 面板控制
    void showPanel(const QString& panelId);
    void hidePanel(const QString& panelId);
    void togglePanel(const QString& panelId);
    void focusPanel(const QString& panelId);

    // ADS管理器访问
    ads::CDockManager* dockManager() const { return m_dockManager; }

    // 面板内容访问
    class ADSPropertyPanel* getADSPropertyPanel() const { return m_adsPropertyPanel; }
    NodePalette* getNodePalette() const { return m_nodePalette; }
    CommandHistoryWidget* getCommandHistoryWidget() const { return m_commandHistoryWidget; }


    
    // 样式设置
    void setupADSStyle();

public slots:
    void resetToDefaultLayout();

signals:
    void panelCreated(const QString& panelId, PanelType type);
    void panelDestroyed(const QString& panelId);
    void panelVisibilityChanged(const QString& panelId, bool visible);
    void panelFocused(const QString& panelId);
    void layoutChanged();
    void layoutPresetChanged(const QString& presetName);

private slots:
    void onPanelClosed(ads::CDockWidget* panel);
    void onPanelOpened(ads::CDockWidget* panel);
    void onFocusChanged(ads::CDockWidget* panel);

private:
    QMainWindow* m_mainWindow;
    ads::CDockManager* m_dockManager;
    
    // 面板注册表
    QHash<QString, ads::CDockWidget*> m_panels;
    QHash<QString, PanelType> m_panelTypes;
    
    // 布局预设
    QHash<QString, QJsonObject> m_layoutPresets;
    
    // 面板组件
    class ADSPropertyPanel* m_adsPropertyPanel;  // ADS专用的轻量级属性面板
    NodePalette* m_nodePalette;
    CommandHistoryWidget* m_commandHistoryWidget;
    
    // 初始化方法
    void setupDockManager();
    void setupPanelConnections();
    void loadLayoutPresets();
    void saveLayoutPresets();
    
    // 辅助方法
    QString getPanelTitle(PanelType type) const;
    QIcon getPanelIcon(PanelType type) const;

    // 面板创建辅助
    QWidget* createPanelContent(PanelType type);
    void configurePanelProperties(ads::CDockWidget* panel, PanelType type);
    
    // 专用面板内容创建
    QWidget* createOutputConsoleWidget();
    QWidget* createProjectExplorerWidget();
};
