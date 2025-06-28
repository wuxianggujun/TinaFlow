#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#include "OpenExcelModel.hpp"
#include "SelectSheetModel.hpp"
#include "ReadCellModel.hpp"
#include "DisplayCellModel.hpp"
#include "ReadRangeModel.hpp"
#include "DisplayRangeModel.hpp"
#include "StringCompareModel.hpp"
#include "DisplayBooleanModel.hpp"
#include "ForEachRowModel.hpp"
#include "DisplayRowModel.hpp"
#include <QtNodes/ConnectionStyle>
#include <QtNodes/DataFlowGraphicsScene>

#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupNodeEditor();
    connectMenuActions();
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
    ret->registerModel<DisplayCellModel>("显示单元格");
    ret->registerModel<ReadRangeModel>("读取范围");
    ret->registerModel<DisplayRangeModel>("显示范围");
    ret->registerModel<StringCompareModel>("条件判断");
    ret->registerModel<DisplayBooleanModel>("显示布尔值");
    ret->registerModel<ForEachRowModel>("遍历行");
    ret->registerModel<DisplayRowModel>("显示行");
    return ret;
}

void MainWindow::connectMenuActions()
{
    // 连接菜单动作
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onActionNew);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onActionOpen);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::onActionSave);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onActionExit);
}

void MainWindow::onActionNew()
{
    // 清空当前图形 - 删除所有节点
    if (m_graphModel) {
        auto nodeIds = m_graphModel->allNodeIds();
        for (const auto& nodeId : nodeIds) {
            m_graphModel->deleteNode(nodeId);
        }
    }
    setWindowTitle("TinaFlow - 新建");
}

void MainWindow::onActionOpen()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("打开流程文件"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("TinaFlow文件 (*.tflow);;JSON文件 (*.json);;所有文件 (*)")
    );

    if (!fileName.isEmpty()) {
        loadFromFile(fileName);
    }
}

void MainWindow::onActionSave()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("保存流程文件"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("TinaFlow文件 (*.tflow);;JSON文件 (*.json)")
    );

    if (!fileName.isEmpty()) {
        saveToFile(fileName);
    }
}

void MainWindow::onActionExit()
{
    close();
}

void MainWindow::saveToFile(const QString& fileName)
{
    if (!m_graphModel) {
        QMessageBox::warning(this, tr("错误"), tr("没有可保存的数据"));
        return;
    }

    try {
        // 使用QtNodes的保存功能
        QJsonObject jsonObject = m_graphModel->save();
        QJsonDocument jsonDocument(jsonObject);

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(jsonDocument.toJson());
            file.close();

            setWindowTitle(QString("TinaFlow - %1").arg(QFileInfo(fileName).baseName()));
            ui->statusbar->showMessage(tr("文件已保存: %1").arg(fileName), 3000);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("无法保存文件: %1").arg(fileName));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("错误"), tr("保存文件时发生错误: %1").arg(e.what()));
    }
}

void MainWindow::loadFromFile(const QString& fileName)
{
    if (!m_graphModel) {
        QMessageBox::warning(this, tr("错误"), tr("图形模型未初始化"));
        return;
    }

    try {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
            if (jsonDocument.isNull()) {
                QMessageBox::critical(this, tr("错误"), tr("文件格式无效"));
                return;
            }

            // 清空当前场景 - 删除所有节点
            auto nodeIds = m_graphModel->allNodeIds();
            for (const auto& nodeId : nodeIds) {
                m_graphModel->deleteNode(nodeId);
            }

            // 加载新数据
            m_graphModel->load(jsonDocument.object());

            setWindowTitle(QString("TinaFlow - %1").arg(QFileInfo(fileName).baseName()));
            ui->statusbar->showMessage(tr("文件已加载: %1").arg(fileName), 3000);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("无法打开文件: %1").arg(fileName));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("错误"), tr("加载文件时发生错误: %1").arg(e.what()));
    }
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
