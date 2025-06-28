#pragma once


#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/GraphicsView>
#include <QtNodes/DataFlowGraphModel>

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

private:
    void setupNodeEditor();
    void setupToolbar();
    void setupPropertyPanel();
    void connectMenuActions();
    void saveToFile(const QString& fileName);
    void loadFromFile(const QString& fileName);
    void setGlobalExecutionState(bool running);
    void triggerDataFlow();
    void updatePropertyPanel(QtNodes::NodeId nodeId);
    void clearPropertyPanel();

    // 特定节点的属性编辑器
    void addForEachRowProperties(QVBoxLayout* layout, QtNodes::NodeId nodeId);
    void addStringCompareProperties(QVBoxLayout* layout, QtNodes::NodeId nodeId);

    static std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registerDataModels();
    static void setStyle();

public:
    static bool isGlobalExecutionEnabled() { return s_globalExecutionEnabled; }

private:
    static bool s_globalExecutionEnabled;
    
    Ui::MainWindow *ui;

    std::unique_ptr<QtNodes::DataFlowGraphModel> m_graphModel;

    // 属性面板相关（使用UI中现有的tab_properties）
    QWidget* m_currentPropertyWidget;
};
