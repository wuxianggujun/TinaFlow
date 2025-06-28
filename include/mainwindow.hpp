#pragma once


#include <QMainWindow>
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

private:
    void setupNodeEditor();
    void connectMenuActions();
    void saveToFile(const QString& fileName);
    void loadFromFile(const QString& fileName);

    static std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registerDataModels();
    static void setStyle();
    
    Ui::MainWindow *ui;

    std::unique_ptr<QtNodes::DataFlowGraphModel> m_graphModel;
};
