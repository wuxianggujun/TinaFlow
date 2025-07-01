#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QKeySequence>
#include <functional>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/ConnectionIdUtils>
#include "TinaFlowGraphicsView.hpp"
#include "widget/ModernToolBar.hpp"
#include "widget/ADSPanelManager.hpp"

// 前向声明
class NodePalette;
namespace Ui {
class MainWindow;
}
namespace ads {
class CDockManager;
class CDockWidget;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    // 常量定义
    struct Constants {
        static constexpr int MIN_WINDOW_WIDTH = 800;
        static constexpr int MIN_WINDOW_HEIGHT = 600;
        static constexpr int DEFAULT_WINDOW_WIDTH = 1200;
        static constexpr int DEFAULT_WINDOW_HEIGHT = 800;
        static constexpr int STATUS_MESSAGE_TIMEOUT = 3000;
        static constexpr int NODE_DUPLICATE_OFFSET = 50;
        static constexpr int UPDATE_THROTTLE_MS = 100;

        static inline const QString WINDOW_TITLE_PREFIX = "TinaFlow";
        static inline const QString FILE_FILTER = "TinaFlow文件 (*.tflow);;JSON文件 (*.json);;所有文件 (*)";
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件操作
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    
    // 执行控制
    void onRunClicked();
    void onPauseClicked();
    void onStopClicked();
    void onUndoClicked();
    void onRedoClicked();
    void onNodeSelected(QtNodes::NodeId nodeId);

    // 右键菜单相关
    void showNodeContextMenu(QtNodes::NodeId nodeId, const QPointF& pos);
    void showConnectionContextMenu(QtNodes::ConnectionId connectionId, const QPointF& pos);
    void deleteSelectedNode();
    void deleteSelectedConnection();
    void showAllConnectionsForDeletion();
    void duplicateSelectedNode();
    
    // 节点面板相关
    void onNodePaletteCreationRequested(const QString& nodeId);
    void onNodePaletteSelectionChanged(const QString& nodeId);
    void showImprovedSceneContextMenu(const QPointF& pos);
    void createNodeFromPalette(const QString& nodeId, const QPointF& position);

private:
    // 节点编辑器设置
    void setupNodeEditor();
    void createGraphicsComponents();
    void connectNodeEditorSignals();
    void connectDataUpdateSignals();
    void reinitializeNodeEditor();
    void cleanupGraphicsComponents();

    // UI设置
    void setupModernToolbar();
    void setupAdvancedPanels();
    void setupKeyboardShortcuts();
    void setupLayoutMenu();
    void setupFileMenu();
    void setupEditMenu();
    void setupViewMenu();
    void createADSLayoutMenu(QMenu* parentMenu);
    void createViewControlMenu(QMenu* parentMenu);
    void saveCurrentLayout();
    QAction* createMenuAction(QMenu* menu, const QString& text,
                             const QKeySequence& shortcut = QKeySequence(),
                             std::function<void()> slot = nullptr);
    void setupWindowDisplay();
    
    // ADS面板更新方法
    void updateADSPropertyPanel(QtNodes::NodeId nodeId);
    void clearADSPropertyPanel();
    void updatePropertyPanelReference();
    void connectADSNodePaletteSignals();
    void setupADSCentralWidget();
    bool validateADSComponents();
    void createADSCentralWidget(ads::CDockManager* dockManager);
    void configureCentralWidgetFeatures(ads::CDockWidget* centralDockWidget);
    void setupCustomStyles();

    // 文件操作封装
    bool saveToFile(const QString& fileName);
    bool loadFromFile(const QString& fileName);
    void handleFileError(const QString& operation, const QString& fileName, const QString& error);

    void setGlobalExecutionState(bool running);
    void triggerDataFlow();
    
    // 端口类型描述方法
    QString getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType, QtNodes::PortIndex portIndex);

    // 改进的节点选中检测方法
    QtNodes::NodeId getSelectedNodeIdImproved(const QPointF& pos);

    // 获取指定位置的节点
    QtNodes::NodeId getNodeAtPosition(const QPointF& pos);

    static std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registerDataModels();

public:
    static bool isGlobalExecutionEnabled() { return s_globalExecutionEnabled; }

private:
    static bool s_globalExecutionEnabled;
    
    Ui::MainWindow *ui;

    std::unique_ptr<QtNodes::DataFlowGraphModel> m_graphModel;
    TinaFlowGraphicsView* m_graphicsView;
    QtNodes::DataFlowGraphicsScene* m_graphicsScene;
    
    ModernToolBar* m_modernToolBar;
    ADSPanelManager* m_adsPanelManager;

    // 选中状态
    QtNodes::NodeId m_selectedNodeId;
    QtNodes::ConnectionId m_selectedConnectionId;
};
