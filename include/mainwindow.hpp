#pragma once


#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/ConnectionIdUtils>
#include "TinaFlowGraphicsView.hpp"
#include "widget/CommandHistoryWidget.hpp"
#include "widget/ModernToolBar.hpp"
#include "NodePalette.hpp"

namespace Ui {
class MainWindow;
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
    void onNodeDeselected();

    // 右键菜单相关
    void showNodeContextMenu(QtNodes::NodeId nodeId, const QPointF& pos);
    void showConnectionContextMenu(QtNodes::ConnectionId connectionId, const QPointF& pos);
    void showSceneContextMenu(const QPointF& pos);
    void deleteSelectedNode();
    void deleteSelectedConnection();
    void showAllConnectionsForDeletion();
    void duplicateSelectedNode();
    
    // 节点面板相关
    void onNodePaletteCreationRequested(const QString& nodeId);
    void onNodePaletteSelectionChanged(const QString& nodeId);
    void showImprovedSceneContextMenu(const QPointF& pos);
    void createNodeFromPalette(const QString& nodeId, const QPointF& position);
    QString getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType, QtNodes::PortIndex portIndex);

private:
    void setupNodeEditor();
    void reinitializeNodeEditor();
    void setupModernToolbar();
    void setupPropertyPanel();
    void setupNodePalette();
    void setupKeyboardShortcuts();
    void setupLayoutMenu();
    void setupCustomStyles();
    void saveToFile(const QString& fileName);
    void loadFromFile(const QString& fileName);
    void setGlobalExecutionState(bool running);
    void triggerDataFlow();
    void updatePropertyPanel(QtNodes::NodeId nodeId);
    void refreshCurrentPropertyPanel(); // 刷新当前选中节点的属性面板
    void clearPropertyPanel();
    void createNodeWithCommand(const QString& nodeType, const QPointF& position);



    static std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registerDataModels();
    static void setStyle();

public:
    static bool isGlobalExecutionEnabled() { return s_globalExecutionEnabled; }

private:
    static bool s_globalExecutionEnabled;
    
    Ui::MainWindow *ui;

    std::unique_ptr<QtNodes::DataFlowGraphModel> m_graphModel;
    TinaFlowGraphicsView* m_graphicsView;
    QtNodes::DataFlowGraphicsScene* m_graphicsScene;

    // 属性面板相关（使用UI中现有的tab_properties）
    QWidget* m_currentPropertyWidget;
    
    // 命令历史面板
    CommandHistoryWidget* m_commandHistoryWidget;
    QDockWidget* m_commandHistoryDock;
    
    // 现代化工具栏
    ModernToolBar* m_modernToolBar;
    
    // 节点面板
    NodePalette* m_nodePalette;
    QDockWidget* m_nodePaletteDock;

    // 右键菜单相关
    QtNodes::NodeId m_selectedNodeId;
    QtNodes::ConnectionId m_selectedConnectionId;
};
