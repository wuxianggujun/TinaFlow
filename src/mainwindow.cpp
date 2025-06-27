#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#include "OpenExcelModel.hpp"
#include "SelectSheetModel.hpp"
#include "ReadCellModel.hpp"
#include <QtNodes/ConnectionStyle>
#include <QtNodes/DataFlowGraphicsScene>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupNodeEditor();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupNodeEditor()
{
    // 注册自定义节点
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> modelRegistry = registerDataModels();

    m_graphModel = std::make_unique<QtNodes::DataFlowGraphModel>(modelRegistry);
    auto * dataFlowGraphicsScene = new QtNodes::DataFlowGraphicsScene(
        *m_graphModel, this);

    auto * graphicsView = new QtNodes::GraphicsView(dataFlowGraphicsScene);
    
    QLayout* containerLayout = ui->nodeEditorHost->layout();
    if (!containerLayout) {
        containerLayout = new QVBoxLayout(ui->nodeEditorHost);
        containerLayout->setContentsMargins(0, 0, 0, 0);
    }
    containerLayout->addWidget(graphicsView);
    
}

std::shared_ptr<QtNodes::NodeDelegateModelRegistry> MainWindow::registerDataModels()
{
    auto ret = std::make_shared<QtNodes::NodeDelegateModelRegistry>();
    ret->registerModel<OpenExcelModel>("Excel操作");
    ret->registerModel<SelectSheetModel>("选择Sheet");
    ret->registerModel<ReadCellModel>("读取单元格");
    return ret;
}

void MainWindow::setStyle()
{
    QtNodes::ConnectionStyle::setConnectionStyle(
        R"(
  {
    "ConnectionStyle": {
      "ConstructionColor": "#808080",
      "NormalColor": "#2A82E4",
      "SelectedColor": "#FFA500",
      "SelectedHaloColor": "#91D7FF",
      "HoveredColor": "#1C73C5",
      
      "LineWidth": 3.0,
      "ConstructionLineWidth": 2.0,
      "PointDiameter": 10.0,
      
      "UseDataDefinedColors": true
    }
  }
  )");
}
