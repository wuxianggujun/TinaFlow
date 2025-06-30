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
    void onActionNew();
    void onActionOpen();
    void onActionSave();
    void onActionExit();
    void onRunClicked();
    void onPauseClicked();
    void onStopClicked();
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
    QString getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType, QtNodes::PortIndex portIndex);

private:
    void setupNodeEditor();
    void setupToolbar();
    void setupPropertyPanel();
    void setupCustomStyles();
    void connectMenuActions();
    void saveToFile(const QString& fileName);
    void loadFromFile(const QString& fileName);
    void setGlobalExecutionState(bool running);
    void triggerDataFlow();
    void updatePropertyPanel(QtNodes::NodeId nodeId);
    void refreshCurrentPropertyPanel(); // 刷新当前选中节点的属性面板
    void clearPropertyPanel();



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

    // 右键菜单相关
    QtNodes::NodeId m_selectedNodeId;
    QtNodes::ConnectionId m_selectedConnectionId;
};
