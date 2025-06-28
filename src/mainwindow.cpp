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

// 需要包含具体的节点模型以便调用特定方法
#include "OpenExcelModel.hpp"

#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QToolBar>
#include <QAction>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>

// 静态成员变量定义
bool MainWindow::s_globalExecutionEnabled = false;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupNodeEditor();
    setupToolbar();
    setupPropertyPanel();
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

    // 连接节点选择事件
    connect(dataFlowGraphicsScene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &MainWindow::onNodeSelected);
    connect(dataFlowGraphicsScene, &QtNodes::DataFlowGraphicsScene::nodeClicked,
            this, &MainWindow::onNodeSelected);
    
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

void MainWindow::setupToolbar()
{
    // 创建工具栏
    QToolBar* toolbar = addToolBar(tr("执行控制"));
    toolbar->setObjectName("ExecutionToolbar");

    // 运行按钮
    QAction* runAction = toolbar->addAction(tr("▶ 运行"));
    runAction->setToolTip(tr("开始执行流程 (F5)\n加载文件后点击此按钮开始处理数据"));
    runAction->setShortcut(QKeySequence("F5"));
    connect(runAction, &QAction::triggered, this, &MainWindow::onRunClicked);

    // 暂停按钮
    QAction* pauseAction = toolbar->addAction(tr("⏸ 暂停"));
    pauseAction->setToolTip(tr("暂停执行"));
    pauseAction->setEnabled(false);
    connect(pauseAction, &QAction::triggered, this, &MainWindow::onPauseClicked);

    // 停止按钮
    QAction* stopAction = toolbar->addAction(tr("⏹ 停止"));
    stopAction->setToolTip(tr("停止执行"));
    stopAction->setEnabled(false);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopClicked);

    // 保存按钮引用以便后续控制
    runAction->setObjectName("runAction");
    pauseAction->setObjectName("pauseAction");
    stopAction->setObjectName("stopAction");
}

void MainWindow::setupPropertyPanel()
{
    // 属性面板已经在UI文件中设计好了，这里只需要初始化
    m_currentPropertyWidget = nullptr;
    qDebug() << "MainWindow: Property panel setup completed using UI design";
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
    ui->statusbar->showMessage(tr("新建流程，拖拽节点开始设计"), 0);
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
            ui->statusbar->showMessage(tr("流程已加载，点击运行按钮(F5)开始执行"), 0);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("无法打开文件: %1").arg(fileName));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("错误"), tr("加载文件时发生错误: %1").arg(e.what()));
    }
}

void MainWindow::onRunClicked()
{
    qDebug() << "MainWindow: Run button clicked";
    setGlobalExecutionState(true);

    // 更新工具栏按钮状态
    findChild<QAction*>("runAction")->setEnabled(false);
    findChild<QAction*>("pauseAction")->setEnabled(true);
    findChild<QAction*>("stopAction")->setEnabled(true);

    // 重新触发数据流处理
    triggerDataFlow();

    ui->statusbar->showMessage(tr("流程正在运行..."), 0);
}

void MainWindow::onPauseClicked()
{
    qDebug() << "MainWindow: Pause button clicked";
    setGlobalExecutionState(false);

    // 更新工具栏按钮状态
    findChild<QAction*>("runAction")->setEnabled(true);
    findChild<QAction*>("pauseAction")->setEnabled(false);
    findChild<QAction*>("stopAction")->setEnabled(true);

    ui->statusbar->showMessage(tr("流程已暂停"), 3000);
}

void MainWindow::onStopClicked()
{
    qDebug() << "MainWindow: Stop button clicked";
    setGlobalExecutionState(false);

    // 更新工具栏按钮状态
    findChild<QAction*>("runAction")->setEnabled(true);
    findChild<QAction*>("pauseAction")->setEnabled(false);
    findChild<QAction*>("stopAction")->setEnabled(false);

    ui->statusbar->showMessage(tr("流程已停止"), 3000);
}

void MainWindow::setGlobalExecutionState(bool running)
{
    s_globalExecutionEnabled = running;
    qDebug() << "MainWindow: Global execution state set to:" << running;

    // 这里可以添加通知所有节点状态变化的逻辑
    // 例如通过信号通知所有节点更新执行状态
}

void MainWindow::onNodeSelected(QtNodes::NodeId nodeId)
{
    qDebug() << "MainWindow: Node selected:" << nodeId;
    updatePropertyPanel(nodeId);
}

void MainWindow::onNodeDeselected()
{
    qDebug() << "MainWindow: Node deselected";
    clearPropertyPanel();
}

void MainWindow::updatePropertyPanel(QtNodes::NodeId nodeId)
{
    if (!m_graphModel) {
        clearPropertyPanel();
        return;
    }

    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (!nodeDelegate) {
        clearPropertyPanel();
        return;
    }

    QString nodeName = nodeDelegate->name();
    QString nodeCaption = nodeDelegate->caption();

    // 更新标题
    ui->nodeTitle->setText(QString("%1 属性").arg(nodeCaption));

    // 清除滚动区域的内容
    QWidget* scrollContent = ui->scrollAreaWidgetContents;
    QVBoxLayout* contentLayout = ui->propertyContentLayout;

    // 清除旧内容
    QLayoutItem* item;
    while ((item = contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 基本信息
    QLabel* infoLabel = new QLabel(QString("节点类型: %1\nID: %2").arg(nodeName).arg(nodeId));
    infoLabel->setStyleSheet("color: #666666; font-size: 11px; padding: 4px;");
    contentLayout->addWidget(infoLabel);

    // 根据节点类型添加特定的属性编辑器
    if (nodeName == "ForEachRow") {
        addForEachRowProperties(contentLayout, nodeId);
    } else if (nodeName == "StringCompare") {
        addStringCompareProperties(contentLayout, nodeId);
    } else {
        // 通用属性
        QLabel* genericLabel = new QLabel(tr("此节点暂无可编辑属性"));
        genericLabel->setAlignment(Qt::AlignCenter);
        genericLabel->setStyleSheet("color: #999999; padding: 20px;");
        contentLayout->addWidget(genericLabel);
    }

    // 添加弹性空间
    contentLayout->addStretch();

    // 切换到属性tab
    ui->rightTab->setCurrentWidget(ui->tab_properties);

    qDebug() << "MainWindow: Updated property panel for node" << nodeId << "(" << nodeCaption << ")";
}

void MainWindow::clearPropertyPanel()
{
    // 重置标题
    ui->nodeTitle->setText(tr("未选择节点"));

    // 清除滚动区域的内容
    QVBoxLayout* contentLayout = ui->propertyContentLayout;

    // 清除旧内容
    QLayoutItem* item;
    while ((item = contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 添加默认内容
    QLabel* defaultLabel = new QLabel(tr("点击节点查看和编辑属性"));
    defaultLabel->setAlignment(Qt::AlignCenter);
    defaultLabel->setStyleSheet("color: #666666; padding: 20px;");
    contentLayout->addWidget(defaultLabel);

    // 添加弹性空间
    contentLayout->addStretch();

    qDebug() << "MainWindow: Cleared property panel";
}

void MainWindow::addForEachRowProperties(QVBoxLayout* layout, QtNodes::NodeId nodeId)
{
    // ForEachRow节点的属性编辑器
    QLabel* descLabel = new QLabel(tr("ForEach循环节点属性："));
    descLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    layout->addWidget(descLabel);

    // 循环模式选择
    QLabel* modeLabel = new QLabel(tr("循环模式："));
    layout->addWidget(modeLabel);

    QComboBox* modeCombo = new QComboBox();
    modeCombo->addItem(tr("逐行处理"), "row_by_row");
    modeCombo->addItem(tr("批量处理"), "batch");
    layout->addWidget(modeCombo);

    // 延迟设置
    QLabel* delayLabel = new QLabel(tr("处理间隔 (毫秒)："));
    layout->addWidget(delayLabel);

    QSpinBox* delaySpinBox = new QSpinBox();
    delaySpinBox->setRange(0, 5000);
    delaySpinBox->setValue(100);
    delaySpinBox->setSuffix(" ms");
    layout->addWidget(delaySpinBox);

    // 最大行数限制
    QLabel* maxRowsLabel = new QLabel(tr("最大处理行数："));
    layout->addWidget(maxRowsLabel);

    QSpinBox* maxRowsSpinBox = new QSpinBox();
    maxRowsSpinBox->setRange(0, 100000);
    maxRowsSpinBox->setValue(0);
    maxRowsSpinBox->setSpecialValueText(tr("无限制"));
    layout->addWidget(maxRowsSpinBox);

    qDebug() << "MainWindow: Added ForEachRow properties for node" << nodeId;
}

void MainWindow::addStringCompareProperties(QVBoxLayout* layout, QtNodes::NodeId nodeId)
{
    // StringCompare节点的属性编辑器
    QLabel* descLabel = new QLabel(tr("字符串比较节点属性："));
    descLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    layout->addWidget(descLabel);

    // 比较操作
    QLabel* operationLabel = new QLabel(tr("比较操作："));
    layout->addWidget(operationLabel);

    QComboBox* operationCombo = new QComboBox();
    operationCombo->addItem(tr("等于"), 0);
    operationCombo->addItem(tr("不等于"), 1);
    operationCombo->addItem(tr("包含"), 2);
    operationCombo->addItem(tr("不包含"), 3);
    operationCombo->addItem(tr("开始于"), 4);
    operationCombo->addItem(tr("结束于"), 5);
    layout->addWidget(operationCombo);

    // 比较值
    QLabel* valueLabel = new QLabel(tr("比较值："));
    layout->addWidget(valueLabel);

    QLineEdit* valueEdit = new QLineEdit();
    valueEdit->setPlaceholderText(tr("输入要比较的值"));
    layout->addWidget(valueEdit);

    // 大小写敏感
    QCheckBox* caseSensitiveCheck = new QCheckBox(tr("区分大小写"));
    layout->addWidget(caseSensitiveCheck);

    qDebug() << "MainWindow: Added StringCompare properties for node" << nodeId;
}

void MainWindow::triggerDataFlow()
{
    qDebug() << "MainWindow: Triggering data flow";

    if (!m_graphModel) {
        qDebug() << "MainWindow: No graph model available";
        return;
    }

    // 获取所有节点ID
    auto nodeIds = m_graphModel->allNodeIds();
    qDebug() << "MainWindow: Found" << nodeIds.size() << "nodes";

    // 遍历所有节点，找到源节点（没有输入连接的节点）并触发它们
    for (const auto& nodeId : nodeIds) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;

        // 检查是否为源节点（没有输入端口或输入端口没有连接）
        bool isSourceNode = true;
        unsigned int inputPorts = nodeDelegate->nPorts(QtNodes::PortType::In);

        for (unsigned int portIndex = 0; portIndex < inputPorts; ++portIndex) {
            auto connections = m_graphModel->connections(nodeId, QtNodes::PortType::In, portIndex);
            if (!connections.empty()) {
                isSourceNode = false;
                break;
            }
        }

        if (isSourceNode) {
            QString nodeName = nodeDelegate->name();
            qDebug() << "MainWindow: Found source node:" << nodeName;

            // 根据节点类型调用特定的触发方法
            if (nodeName == "OpenExcel") {
                auto* openExcelModel = m_graphModel->delegateModel<OpenExcelModel>(nodeId);
                if (openExcelModel) {
                    qDebug() << "MainWindow: Triggering OpenExcelModel execution";
                    openExcelModel->triggerExecution();
                }
            }

            // 触发源节点的数据更新
            for (unsigned int portIndex = 0; portIndex < nodeDelegate->nPorts(QtNodes::PortType::Out); ++portIndex) {
                emit nodeDelegate->dataUpdated(portIndex);
            }
        }
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
