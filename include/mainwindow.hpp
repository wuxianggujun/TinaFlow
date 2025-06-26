#pragma once


#include <QMainWindow>
#include <QtNodes/NodeDelegateModelRegistry>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    void setupNodeEditor();

    static std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registerDataModels();

    static  void setStyle();
    
    Ui::MainWindow *ui;
};
