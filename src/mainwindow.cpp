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

#include "DisplayRowModel.hpp"
#include "RangeInfoModel.hpp"
#include "SmartLoopProcessorModel.hpp"
#include "DisplayCellListModel.hpp"
#include "SaveExcelModel.hpp"
#include <QtNodes/ConnectionStyle>
#include <QtNodes/NodeStyle>
#include <QtNodes/DataFlowGraphicsScene>



#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <limits>
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
    m_graphicsScene = new QtNodes::DataFlowGraphicsScene(*m_graphModel, this);
    m_graphicsView = new TinaFlowGraphicsView(m_graphicsScene, this);

    // 应用自定义样式
    setupCustomStyles();

    // 连接节点选择事件
    connect(m_graphicsScene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &MainWindow::onNodeSelected);
    connect(m_graphicsScene, &QtNodes::DataFlowGraphicsScene::nodeClicked,
            this, &MainWindow::onNodeSelected);

    // 连接右键菜单事件
    connect(m_graphicsView, &TinaFlowGraphicsView::nodeContextMenuRequested,
            this, &MainWindow::showNodeContextMenu);
    connect(m_graphicsView, &TinaFlowGraphicsView::connectionContextMenuRequested,
            this, &MainWindow::showConnectionContextMenu);
    connect(m_graphicsView, &TinaFlowGraphicsView::sceneContextMenuRequested,
            this, &MainWindow::showSceneContextMenu);
    
    QLayout* containerLayout = ui->nodeEditorHost->layout();
    if (!containerLayout) {
        containerLayout = new QVBoxLayout(ui->nodeEditorHost);
        containerLayout->setContentsMargins(0, 0, 0, 0);
    }
    containerLayout->addWidget(m_graphicsView);
    
}

std::shared_ptr<QtNodes::NodeDelegateModelRegistry> MainWindow::registerDataModels()
{
    auto ret = std::make_shared<QtNodes::NodeDelegateModelRegistry>();
    // 使用英文名称注册（与节点的name()方法返回值一致）
    ret->registerModel<OpenExcelModel>("OpenExcel");
    ret->registerModel<SelectSheetModel>("SelectSheet");
    ret->registerModel<ReadCellModel>("ReadCell");
    ret->registerModel<DisplayCellModel>("DisplayCell");
    ret->registerModel<ReadRangeModel>("ReadRange");
    ret->registerModel<DisplayRangeModel>("DisplayRange");
    ret->registerModel<StringCompareModel>("StringCompare");
    ret->registerModel<DisplayBooleanModel>("DisplayBoolean");

    ret->registerModel<SmartLoopProcessorModel>("SmartLoopProcessor");
    ret->registerModel<DisplayCellListModel>("DisplayCellList");
    ret->registerModel<DisplayRowModel>("DisplayRow");
    ret->registerModel<RangeInfoModel>("RangeInfo");
    ret->registerModel<SaveExcelModel>("SaveExcel");
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
    if (nodeName == "StringCompare") {
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

void MainWindow::setupCustomStyles()
{
    // 自定义节点样式JSON
    QString nodeStyleJson = R"({
        "NodeStyle": {
            "NormalBoundaryColor": [255, 255, 255],
            "SelectedBoundaryColor": [255, 165, 0],
            "GradientColor0": [240, 240, 240],
            "GradientColor1": [220, 220, 220],
            "GradientColor2": [200, 200, 200],
            "GradientColor3": [180, 180, 180],
            "ShadowColor": [20, 20, 20],
            "FontColor": [10, 10, 10],
            "FontColorFaded": [100, 100, 100],
            "ConnectionPointColor": [70, 130, 180],
            "FilledConnectionPointColor": [34, 139, 34],
            "WarningColor": [128, 128, 0],
            "ErrorColor": [255, 50, 50],
            "PenWidth": 2.0,
            "HoveredPenWidth": 2.5,
            "ConnectionPointDiameter": 10.0,
            "Opacity": 1.0
        }
    })";

    // 自定义连接线样式JSON
    QString connectionStyleJson = R"({
        "ConnectionStyle": {
            "ConstructionColor": [169, 169, 169],
            "NormalColor": [100, 100, 100],
            "SelectedColor": [255, 165, 0],
            "SelectedHaloColor": [255, 165, 0, 50],
            "HoveredColor": [136, 136, 136],
            "LineWidth": 3.0,
            "ConstructionLineWidth": 2.0,
            "PointDiameter": 10.0,
            "UseDataDefinedColors": true
        }
    })";

    // 应用样式
    QtNodes::ConnectionStyle::setConnectionStyle(connectionStyleJson);
    QtNodes::NodeStyle::setNodeStyle(nodeStyleJson);

    qDebug() << "MainWindow: Custom styles applied";
}

QString MainWindow::getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType, QtNodes::PortIndex portIndex)
{
    if (!nodeModel) return "Unknown";

    auto dataType = nodeModel->dataType(portType, portIndex);
    QString typeName = dataType.name;

    // 将数据类型转换为更友好的显示名称
    static QMap<QString, QString> typeDescriptions = {
        {"WorkbookData", "工作簿"},
        {"SheetData", "工作表"},
        {"RangeData", "范围数据"},
        {"RowData", "行数据"},
        {"CellData", "单元格"},
        {"BooleanData", "布尔值"}
    };

    return typeDescriptions.value(typeName, typeName);
}

// 右键菜单实现
void MainWindow::showNodeContextMenu(QtNodes::NodeId nodeId, const QPointF& pos)
{
    m_selectedNodeId = nodeId;

    QMenu contextMenu(this);

    QAction* deleteAction = contextMenu.addAction("删除节点");
    deleteAction->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedNode);

    QAction* duplicateAction = contextMenu.addAction("复制节点");
    duplicateAction->setIcon(QIcon(":/icons/copy.png"));
    connect(duplicateAction, &QAction::triggered, this, &MainWindow::duplicateSelectedNode);

    contextMenu.addSeparator();

    // 获取节点信息
    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (nodeDelegate) {
        QAction* infoAction = contextMenu.addAction(QString("节点: %1").arg(nodeDelegate->name()));
        infoAction->setEnabled(false);
    }

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

void MainWindow::showConnectionContextMenu(QtNodes::ConnectionId connectionId, const QPointF& pos)
{
    m_selectedConnectionId = connectionId;

    QMenu contextMenu(this);

    // 获取连接信息
    auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.outNodeId);
    auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.inNodeId);

    if (outNodeDelegate && inNodeDelegate) {
        QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, connectionId.outPortIndex);
        QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, connectionId.inPortIndex);

        QString description = QString("%1[%2:%3] → %4[%5:%6]")
            .arg(outNodeDelegate->name())
            .arg(connectionId.outPortIndex)
            .arg(outPortType)
            .arg(inNodeDelegate->name())
            .arg(connectionId.inPortIndex)
            .arg(inPortType);

        QAction* infoAction = contextMenu.addAction(QString("连接: %1").arg(description));
        infoAction->setEnabled(false);
        contextMenu.addSeparator();
    }

    QAction* deleteAction = contextMenu.addAction("删除连接");
    deleteAction->setIcon(QIcon(":/icons/disconnect.png"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedConnection);

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

// 删除了不稳定的位置查找方法

void MainWindow::showSceneContextMenu(const QPointF& pos)
{
    QMenu contextMenu(this);

    // 添加节点子菜单
    QMenu* addNodeMenu = contextMenu.addMenu("添加节点");
    addNodeMenu->setIcon(QIcon(":/icons/add.png"));

    // 数据源节点
    QMenu* dataSourceMenu = addNodeMenu->addMenu("数据源");
    QAction* addExcelAction = dataSourceMenu->addAction("Excel文件");
    connect(addExcelAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("OpenExcel");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addSelectSheetAction = dataSourceMenu->addAction("选择工作表");
    connect(addSelectSheetAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("SelectSheet");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addReadRangeAction = dataSourceMenu->addAction("读取范围");
    connect(addReadRangeAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("ReadRange");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addReadCellAction = dataSourceMenu->addAction("读取单元格");
    connect(addReadCellAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("ReadCell");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    dataSourceMenu->addSeparator();

    QAction* addSaveExcelAction = dataSourceMenu->addAction("保存Excel");
    connect(addSaveExcelAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("SaveExcel");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    // 处理节点
    QMenu* processMenu = addNodeMenu->addMenu("数据处理");
    QAction* addSmartLoopAction = processMenu->addAction("智能循环处理器");
    connect(addSmartLoopAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("SmartLoopProcessor");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });



    QAction* addStringCompareAction = processMenu->addAction("字符串比较");
    connect(addStringCompareAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("StringCompare");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    // 显示节点
    QMenu* displayMenu = addNodeMenu->addMenu("显示");
    QAction* addDisplayRowAction = displayMenu->addAction("显示行");
    connect(addDisplayRowAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("DisplayRow");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addDisplayBooleanAction = displayMenu->addAction("显示布尔值");
    connect(addDisplayBooleanAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("DisplayBoolean");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addDisplayCellAction = displayMenu->addAction("显示单元格");
    connect(addDisplayCellAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("DisplayCell");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addDisplayCellListAction = displayMenu->addAction("显示单元格列表");
    connect(addDisplayCellListAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("DisplayCellList");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addDisplayRangeAction = displayMenu->addAction("显示范围");
    connect(addDisplayRangeAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("DisplayRange");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    QAction* addRangeInfoAction = displayMenu->addAction("范围信息");
    connect(addRangeInfoAction, &QAction::triggered, [this, pos]() {
        auto nodeId = m_graphModel->addNode("RangeInfo");
        m_graphModel->setNodeData(nodeId, QtNodes::NodeRole::Position, pos);
    });

    contextMenu.addSeparator();

    // 画布操作
    QAction* clearAllAction = contextMenu.addAction("清空画布");
    clearAllAction->setIcon(QIcon(":/icons/clear.png"));
    connect(clearAllAction, &QAction::triggered, [this]() {
        if (QMessageBox::question(this, "确认", "确定要清空所有节点吗？") == QMessageBox::Yes) {
            // 删除所有节点
            auto nodeIds = m_graphModel->allNodeIds();
            for (auto nodeId : nodeIds) {
                m_graphModel->deleteNode(nodeId);
            }
        }
    });

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

void MainWindow::deleteSelectedNode()
{
    if (m_selectedNodeId != QtNodes::InvalidNodeId) {
        m_graphModel->deleteNode(m_selectedNodeId);
        m_selectedNodeId = QtNodes::InvalidNodeId;

        // 清空属性面板
        clearPropertyPanel();
    }
}

void MainWindow::deleteSelectedConnection()
{
    // 检查是否有有效的连接被选中
    if (m_selectedConnectionId.outNodeId == QtNodes::InvalidNodeId ||
        m_selectedConnectionId.inNodeId == QtNodes::InvalidNodeId) {
        QMessageBox::information(this, "提示", "没有选中有效的连接");
        return;
    }

    // 直接删除连接，不需要确认（因为用户已经明确右键点击了特定连接）
    m_graphModel->deleteConnection(m_selectedConnectionId);

    // 获取连接信息用于日志
    auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedConnectionId.outNodeId);
    auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedConnectionId.inNodeId);

    if (outNodeDelegate && inNodeDelegate) {
        QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, m_selectedConnectionId.outPortIndex);
        QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, m_selectedConnectionId.inPortIndex);

        QString description = QString("%1[%2:%3] → %4[%5:%6]")
            .arg(outNodeDelegate->name())
            .arg(m_selectedConnectionId.outPortIndex)
            .arg(outPortType)
            .arg(inNodeDelegate->name())
            .arg(m_selectedConnectionId.inPortIndex)
            .arg(inPortType);

        qDebug() << "MainWindow: Deleted connection:" << description;
    }

    // 重置选中的连接
    m_selectedConnectionId = {QtNodes::InvalidNodeId, QtNodes::InvalidPortIndex,
                             QtNodes::InvalidNodeId, QtNodes::InvalidPortIndex};
}

void MainWindow::showAllConnectionsForDeletion()
{
    auto allNodes = m_graphModel->allNodeIds();
    QStringList connectionList;
    QList<QtNodes::ConnectionId> connections;

    for (auto nodeId : allNodes) {
        auto nodeConnections = m_graphModel->allConnectionIds(nodeId);
        for (auto connectionId : nodeConnections) {
            auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.outNodeId);
            auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.inNodeId);

            if (outNodeDelegate && inNodeDelegate) {
                QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, connectionId.outPortIndex);
                QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, connectionId.inPortIndex);

                QString description = QString("%1[%2:%3] → %4[%5:%6]")
                    .arg(outNodeDelegate->name())
                    .arg(connectionId.outPortIndex)
                    .arg(outPortType)
                    .arg(inNodeDelegate->name())
                    .arg(connectionId.inPortIndex)
                    .arg(inPortType);
                connectionList.append(description);
                connections.append(connectionId);
            }
        }
    }

    if (connections.isEmpty()) {
        QMessageBox::information(this, "提示", "没有找到可删除的连接");
        return;
    }

    bool ok;
    QString selectedConnection = QInputDialog::getItem(this, "删除连接",
        "选择要删除的连接:", connectionList, 0, false, &ok);

    if (ok && !selectedConnection.isEmpty()) {
        int index = connectionList.indexOf(selectedConnection);
        if (index >= 0 && index < connections.size()) {
            m_graphModel->deleteConnection(connections[index]);
            qDebug() << "MainWindow: Deleted connection:" << selectedConnection;
        }
    }
}

void MainWindow::duplicateSelectedNode()
{
    if (m_selectedNodeId != QtNodes::InvalidNodeId) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedNodeId);
        if (nodeDelegate) {
            // 创建新节点
            auto newNodeId = m_graphModel->addNode(nodeDelegate->name());

            // 获取原节点位置并偏移
            QVariant posVariant = m_graphModel->nodeData(m_selectedNodeId, QtNodes::NodeRole::Position);
            QPointF originalPos = posVariant.toPointF();
            QPointF newPos = originalPos + QPointF(50, 50); // 偏移50像素
            m_graphModel->setNodeData(newNodeId, QtNodes::NodeRole::Position, newPos);

            // TODO: 复制节点的属性设置
        }
    }
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

        QString nodeName = nodeDelegate->name();



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
