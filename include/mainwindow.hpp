#pragma once

#include <QMainWindow>
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
    void setupNodeEditor();
    void reinitializeNodeEditor();
    void cleanupGraphicsComponents();
    void setupModernToolbar();
    void setupAdvancedPanels();
    void setupKeyboardShortcuts();
    void setupLayoutMenu();
    void setupFileMenu(QMenuBar* menuBar);
    void setupViewMenu(QMenuBar* menuBar);
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
    void saveToFile(const QString& fileName);
    void loadFromFile(const QString& fileName);
    void setGlobalExecutionState(bool running);
    void triggerDataFlow();
    
    // 端口类型描述方法
    QString getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType, QtNodes::PortIndex portIndex);
    
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
