#include "mainwindow.hpp"
#include "ui_mainwindow.h"

// 核心节点模型
#include "OpenExcelModel.hpp"
#include "SelectSheetModel.hpp"
#include "ReadCellModel.hpp"
#include "ReadRangeModel.hpp"
#include "SaveExcelModel.hpp"
#include "SmartLoopProcessorModel.hpp"
#include "StringCompareModel.hpp"

// 显示节点模型
#include "DisplayCellModel.hpp"
#include "DisplayRangeModel.hpp"
#include "DisplayBooleanModel.hpp"
#include "DisplayRowModel.hpp"
#include "DisplayCellListModel.hpp"
#include "RangeInfoModel.hpp"

// 系统组件
#include "IPropertyProvider.hpp"
#include "PropertyWidget.hpp"
#include "ErrorHandler.hpp"
#include "DataValidator.hpp"
#include "CommandManager.hpp"
#include "NodeCommands.hpp"
#include "ModernToolBar.hpp"

// QtNodes
#include <QtNodes/ConnectionStyle>
#include <QtNodes/NodeStyle>
#include <QtNodes/DataFlowGraphicsScene>

// Qt核心
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QInputDialog>

// Qt界面
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>

// 静态成员变量定义
bool MainWindow::s_globalExecutionEnabled = false;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    setupNodeEditor();
    setupModernToolbar();
    setupPropertyPanel();
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

    // 连接数据更新事件，用于刷新属性面板
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::inPortDataWasSet,
            this, [this](QtNodes::NodeId nodeId, QtNodes::PortType, QtNodes::PortIndex) {
                // 如果更新的节点是当前选中的节点，刷新属性面板
                if (nodeId == m_selectedNodeId) {
                    refreshCurrentPropertyPanel();
                }
            });

    // 连接节点更新事件
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeUpdated,
            this, [this](QtNodes::NodeId nodeId) {
                // 如果更新的节点是当前选中的节点，刷新属性面板
                if (nodeId == m_selectedNodeId) {
                    refreshCurrentPropertyPanel();
                }
            });
    

    
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

void MainWindow::setupModernToolbar()
{
    // 创建现代化工具栏
    m_modernToolBar = new ModernToolBar(this);
    
    // 将工具栏添加到主窗口
    addToolBar(Qt::TopToolBarArea, m_modernToolBar);
    
    // 连接文件操作信号
    connect(m_modernToolBar, &ModernToolBar::newFileRequested, this, &MainWindow::onNewFile);
    connect(m_modernToolBar, &ModernToolBar::openFileRequested, this, &MainWindow::onOpenFile);
    connect(m_modernToolBar, &ModernToolBar::saveFileRequested, this, &MainWindow::onSaveFile);
    
    // 连接编辑操作信号
    connect(m_modernToolBar, &ModernToolBar::undoRequested, this, &MainWindow::onUndoClicked);
    connect(m_modernToolBar, &ModernToolBar::redoRequested, this, &MainWindow::onRedoClicked);
    
    // 连接执行控制信号
    connect(m_modernToolBar, &ModernToolBar::runRequested, this, &MainWindow::onRunClicked);
    connect(m_modernToolBar, &ModernToolBar::pauseRequested, this, &MainWindow::onPauseClicked);
    connect(m_modernToolBar, &ModernToolBar::stopRequested, this, &MainWindow::onStopClicked);

    
    // 连接视图控制信号
    connect(m_modernToolBar, &ModernToolBar::zoomFitRequested, this, [this](){
        if (m_graphicsView) {
            m_graphicsView->fitInView(m_graphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    });
    connect(m_modernToolBar, &ModernToolBar::zoomInRequested, this, [this](){
        if (m_graphicsView) {
            m_graphicsView->scale(1.2, 1.2);
        }
    });
    connect(m_modernToolBar, &ModernToolBar::zoomOutRequested, this, [this](){
        if (m_graphicsView) {
            m_graphicsView->scale(0.8, 0.8);
        }
    });

    
    // 连接命令管理器信号到工具栏（使用统一信号避免重入问题）
    auto& commandManager = CommandManager::instance();
    connect(&commandManager, &CommandManager::undoRedoStateChanged, [this](bool canUndo, bool canRedo){
        if (m_modernToolBar) {
            m_modernToolBar->updateUndoRedoState(canUndo, canRedo);
        }
    });
    
    // 初始化状态
    m_modernToolBar->updateExecutionState(false);
    m_modernToolBar->updateUndoRedoState(false, false);
}

void MainWindow::setupPropertyPanel()
{
    // 属性面板已经在UI文件中设计好了，这里只需要初始化
    m_currentPropertyWidget = nullptr;
    
    // 创建命令历史标签页
    m_commandHistoryWidget = new CommandHistoryWidget(this);
    ui->rightTab->addTab(m_commandHistoryWidget, "命令历史");
    
    qDebug() << "MainWindow: Property panel setup completed using UI design";
}



void MainWindow::onNewFile()
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

void MainWindow::onOpenFile()
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

void MainWindow::onSaveFile()
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

    // 更新现代化工具栏状态
    if (m_modernToolBar) {
        m_modernToolBar->updateExecutionState(true);
    }

    // 重新触发数据流处理
    triggerDataFlow();

    ui->statusbar->showMessage(tr("流程正在运行..."), 0);
}

void MainWindow::onPauseClicked()
{
    qDebug() << "MainWindow: Pause button clicked";
    setGlobalExecutionState(false);

    // 更新现代化工具栏状态
    if (m_modernToolBar) {
        m_modernToolBar->updateExecutionState(false);
    }

    ui->statusbar->showMessage(tr("流程已暂停"), 3000);
}

void MainWindow::onStopClicked()
{
    qDebug() << "MainWindow: Stop button clicked";
    setGlobalExecutionState(false);

    // 更新现代化工具栏状态
    if (m_modernToolBar) {
        m_modernToolBar->updateExecutionState(false);
    }

    ui->statusbar->showMessage(tr("流程已停止"), 3000);
}

void MainWindow::onUndoClicked()
{
    qDebug() << "MainWindow: Undo clicked";
    auto& commandManager = CommandManager::instance();
    if (commandManager.canUndo()) {
        if (commandManager.undo()) {
            ui->statusbar->showMessage(tr("已撤销: %1").arg(commandManager.getUndoText()), 2000);
        } else {
            ui->statusbar->showMessage(tr("撤销失败"), 2000);
        }
    }
}

void MainWindow::onRedoClicked()
{
    qDebug() << "MainWindow: Redo clicked";
    auto& commandManager = CommandManager::instance();
    if (commandManager.canRedo()) {
        if (commandManager.redo()) {
            ui->statusbar->showMessage(tr("已重做: %1").arg(commandManager.getRedoText()), 2000);
        } else {
            ui->statusbar->showMessage(tr("重做失败"), 2000);
        }
    }
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
    m_selectedNodeId = nodeId; // 保存选中的节点ID
    updatePropertyPanel(nodeId);
}

void MainWindow::onNodeDeselected()
{
    qDebug() << "MainWindow: Node deselected";
    m_selectedNodeId = QtNodes::NodeId{}; // 清除选中的节点ID
    clearPropertyPanel();
}

void MainWindow::refreshCurrentPropertyPanel()
{
    // 如果有选中的节点，刷新其属性面板
    if (m_selectedNodeId != QtNodes::NodeId{}) {
        qDebug() << "MainWindow: Refreshing property panel for node" << m_selectedNodeId;
        updatePropertyPanel(m_selectedNodeId);
    }
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

    // 尝试使用新的属性提供系统
    bool hasProperties = false;

    // 获取节点模型并检查是否实现了IPropertyProvider接口
    auto* nodeModel = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (nodeModel) {
        auto* propertyProvider = dynamic_cast<IPropertyProvider*>(nodeModel);
        if (propertyProvider) {
            // 使用新的PropertyWidget系统
            auto* propertyWidget = PropertyPanelManager::createPropertyPanel(contentLayout);
            hasProperties = propertyProvider->createPropertyPanel(propertyWidget);
            qDebug() << "MainWindow: Used PropertyWidget system for node" << nodeId;
        }
    }

    // 如果没有属性提供者，显示默认消息
    if (!hasProperties) {
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

void MainWindow::createNodeWithCommand(const QString& nodeType, const QPointF& position)
{
    auto command = std::make_unique<CreateNodeCommand>(m_graphicsScene, nodeType, position);
    auto& commandManager = CommandManager::instance();
    
    if (commandManager.executeCommand(std::move(command))) {
        ui->statusbar->showMessage(tr("已创建 %1 节点").arg(nodeType), 2000);
    } else {
        ui->statusbar->showMessage(tr("创建节点失败"), 2000);
    }
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
        createNodeWithCommand("OpenExcel", pos);
    });

    QAction* addSelectSheetAction = dataSourceMenu->addAction("选择工作表");
    connect(addSelectSheetAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("SelectSheet", pos);
    });

    QAction* addReadRangeAction = dataSourceMenu->addAction("读取范围");
    connect(addReadRangeAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("ReadRange", pos);
    });

    QAction* addReadCellAction = dataSourceMenu->addAction("读取单元格");
    connect(addReadCellAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("ReadCell", pos);
    });

    dataSourceMenu->addSeparator();

    QAction* addSaveExcelAction = dataSourceMenu->addAction("保存Excel");
    connect(addSaveExcelAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("SaveExcel", pos);
    });

    // 处理节点
    QMenu* processMenu = addNodeMenu->addMenu("数据处理");
    QAction* addSmartLoopAction = processMenu->addAction("智能循环处理器");
    connect(addSmartLoopAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("SmartLoopProcessor", pos);
    });

    QAction* addStringCompareAction = processMenu->addAction("字符串比较");
    connect(addStringCompareAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("StringCompare", pos);
    });

    // 显示节点
    QMenu* displayMenu = addNodeMenu->addMenu("显示");
    QAction* addDisplayRowAction = displayMenu->addAction("显示行");
    connect(addDisplayRowAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("DisplayRow", pos);
    });

    QAction* addDisplayBooleanAction = displayMenu->addAction("显示布尔值");
    connect(addDisplayBooleanAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("DisplayBoolean", pos);
    });

    QAction* addDisplayCellAction = displayMenu->addAction("显示单元格");
    connect(addDisplayCellAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("DisplayCell", pos);
    });

    QAction* addDisplayCellListAction = displayMenu->addAction("显示单元格列表");
    connect(addDisplayCellListAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("DisplayCellList", pos);
    });

    QAction* addDisplayRangeAction = displayMenu->addAction("显示范围");
    connect(addDisplayRangeAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("DisplayRange", pos);
    });

    QAction* addRangeInfoAction = displayMenu->addAction("范围信息");
    connect(addRangeInfoAction, &QAction::triggered, [this, pos]() {
        createNodeWithCommand("RangeInfo", pos);
    });

    contextMenu.addSeparator();

    // 画布操作
    QAction* clearAllAction = contextMenu.addAction("清空画布");
    clearAllAction->setIcon(QIcon(":/icons/clear.png"));
    connect(clearAllAction, &QAction::triggered, [this]() {
        if (QMessageBox::question(this, "确认", "确定要清空所有节点吗？") == QMessageBox::Yes) {
            auto nodeIds = m_graphModel->allNodeIds();
            if (!nodeIds.empty()) {
                // 使用宏命令批量删除所有节点
                auto& commandManager = CommandManager::instance();
                commandManager.beginMacro("清空画布");
                
            for (auto nodeId : nodeIds) {
                    auto command = std::make_unique<DeleteNodeCommand>(m_graphicsScene, nodeId);
                    commandManager.executeCommand(std::move(command));
                }
                
                commandManager.endMacro();
                ui->statusbar->showMessage(tr("已清空画布，删除了 %1 个节点").arg(nodeIds.size()), 3000);
            }
        }
    });

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

void MainWindow::deleteSelectedNode()
{
    if (m_selectedNodeId != QtNodes::NodeId{}) {
        // 使用命令系统删除节点
        auto command = std::make_unique<DeleteNodeCommand>(m_graphicsScene, m_selectedNodeId);
        auto& commandManager = CommandManager::instance();
        
        if (commandManager.executeCommand(std::move(command))) {
            m_selectedNodeId = QtNodes::NodeId{};
        // 清空属性面板
        clearPropertyPanel();
            ui->statusbar->showMessage(tr("节点已删除"), 2000);
        } else {
            ui->statusbar->showMessage(tr("删除节点失败"), 2000);
        }
    }
}

void MainWindow::deleteSelectedConnection()
{
    // 检查是否有有效的连接被选中（简化检查）
    // 实际应用中需要根据QtNodes的ConnectionId结构来检查
    bool hasValidConnection = true; // 简化处理
    
    if (!hasValidConnection) {
        QMessageBox::information(this, "提示", "没有选中有效的连接");
        return;
    }

    // 使用命令系统删除连接
    auto command = std::make_unique<DeleteConnectionCommand>(m_graphicsScene, m_selectedConnectionId);
    auto& commandManager = CommandManager::instance();

    // 获取连接信息用于显示
    auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedConnectionId.outNodeId);
    auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedConnectionId.inNodeId);
    QString description = "连接";

    if (outNodeDelegate && inNodeDelegate) {
        QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, m_selectedConnectionId.outPortIndex);
        QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, m_selectedConnectionId.inPortIndex);

        description = QString("%1[%2:%3] → %4[%5:%6]")
            .arg(outNodeDelegate->name())
            .arg(m_selectedConnectionId.outPortIndex)
            .arg(outPortType)
            .arg(inNodeDelegate->name())
            .arg(m_selectedConnectionId.inPortIndex)
            .arg(inPortType);
    }

    if (commandManager.executeCommand(std::move(command))) {
        qDebug() << "MainWindow: Deleted connection:" << description;
        ui->statusbar->showMessage(tr("连接已删除: %1").arg(description), 3000);
    } else {
        ui->statusbar->showMessage(tr("删除连接失败"), 2000);
    }

    // 重置选中的连接
    m_selectedConnectionId = QtNodes::ConnectionId{};
}

void MainWindow::showAllConnectionsForDeletion()
{
    auto allNodes = m_graphModel->allNodeIds();
    QStringList connectionList;
    QList<QtNodes::ConnectionId> connections;

    for (auto nodeId : allNodes) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;
        
        // 检查所有输出端口的连接
        unsigned int outputPorts = nodeDelegate->nPorts(QtNodes::PortType::Out);
        for (unsigned int portIndex = 0; portIndex < outputPorts; ++portIndex) {
            auto nodeConnections = m_graphModel->connections(nodeId, QtNodes::PortType::Out, portIndex);
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
    if (m_selectedNodeId != QtNodes::NodeId{}) {
        // 简化的复制实现
        // 实际应用中需要获取节点类型和属性

            // 获取原节点位置并偏移
            QVariant posVariant = m_graphModel->nodeData(m_selectedNodeId, QtNodes::NodeRole::Position);
            QPointF originalPos = posVariant.toPointF();
            QPointF newPos = originalPos + QPointF(50, 50); // 偏移50像素
        
        // 使用命令系统创建新节点（暂时使用固定类型）
        auto command = std::make_unique<CreateNodeCommand>(m_graphicsScene, "DisplayCell", newPos);
        auto& commandManager = CommandManager::instance();
        
        if (commandManager.executeCommand(std::move(command))) {
            ui->statusbar->showMessage(tr("节点已复制"), 2000);
            // TODO: 复制节点的属性设置
        } else {
            ui->statusbar->showMessage(tr("复制节点失败"), 2000);
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
